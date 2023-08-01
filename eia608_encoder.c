#include "eia608_encoder.h"
#include <stdio.h>
#include "mpeg.h"

#define DEFAULT_CHANNEL 0
#define PADDING_COMMAND 0x8080

libcaption_stauts_t encode_cc(sei_t* sei, double timestamp, uint16_t cc_element, int cc_count)
{
    int j;
    cea708_t cea708;
    cea708_init(&cea708, timestamp); // set up a new popon frame
    cea708_add_cc_data(&cea708, 1, cc_type_ntsc_cc_field_1, cc_element);
    for (j = 1; j < cc_count; ++j) {
        cea708_add_cc_data(&cea708, 0, 0, 0);
    }
    // cea708_dump(&cea708);
    sei_append(sei, &cea708);
    return LIBCAPTION_OK;
}

void sei_append(sei_t* sei, cea708_t* cea708)
{
    sei_message_t* msg = sei_message_new(sei_type_user_data_registered_itu_t_t35, 0, CEA608_MAX_SIZE);
    msg->size = cea708_render(cea708, sei_message_data(msg), sei_message_size(msg));
    sei_message_append(sei, msg);
    cea708_init(cea708, sei->timestamp); // will confgure using HLS compatiable defaults
}

libcaption_stauts_t sei_for_padding(sei_t* sei, int cc_count)
{
    encode_cc(sei, 1, PADDING_COMMAND, cc_count);
    return LIBCAPTION_OK;
}

libcaption_stauts_t sei_for_display(sei_t* sei, int cc_count)
{
    encode_cc(sei, 1, eia608_control_command(eia608_control_erase_display_memory, DEFAULT_CHANNEL), cc_count);
    encode_cc(sei, 1, eia608_control_command(eia608_control_end_of_caption, DEFAULT_CHANNEL), cc_count);
    return LIBCAPTION_OK;
}

libcaption_stauts_t sei_for_clear(sei_t* sei, int cc_count)
{
    encode_cc(sei, 1, eia608_control_command(eia608_control_erase_display_memory, DEFAULT_CHANNEL), cc_count);
    return LIBCAPTION_OK;
}

////////////////////////////////////////////////////////////////////////////////
// logic of this function is the same as sei_from_caption_frame in mpeg.c
libcaption_stauts_t sei_for_captions(sei_t* sei, caption_frame_t* frame, int cc_count)
{

    int r, c;
    int unl, prev_unl;
//    cea708_t cea708;
    const char* data;
    uint16_t prev_cc_data;
    eia608_style_t styl, prev_styl;

    sei_init(sei, frame->timestamp);
    encode_cc(sei, frame->timestamp, eia608_control_command(eia608_control_erase_non_displayed_memory, DEFAULT_CHANNEL), cc_count);
    encode_cc(sei, frame->timestamp, eia608_control_command(eia608_control_resume_caption_loading, DEFAULT_CHANNEL), cc_count);

    for (r = 0; r < SCREEN_ROWS; ++r) {
        prev_unl = 0, prev_styl = eia608_style_white;
        // Calculate preamble
        for (c = 0; c < SCREEN_COLS && 0 == *caption_frame_read_char(frame, r, c, &styl, &unl); ++c) {
        }

        // This row is blank
        if (SCREEN_COLS == c) {
            continue;
        }

        // Write preamble
        if (0 < c || (0 == unl && eia608_style_white == styl)) {
            int tab = c % 4;
            encode_cc(sei, frame->timestamp,  eia608_row_column_pramble(r, c, DEFAULT_CHANNEL, 0), cc_count);
            if (tab) {
               encode_cc(sei, frame->timestamp,  eia608_tab(tab, DEFAULT_CHANNEL), cc_count);
            }
        } else {
            encode_cc(sei, frame->timestamp, eia608_row_style_pramble(r, DEFAULT_CHANNEL, styl, unl), cc_count);
            prev_unl = unl, prev_styl = styl;
        }

        // Write the row
        for (prev_cc_data = 0, data = caption_frame_read_char(frame, r, c, 0, 0);
             (*data) && c < SCREEN_COLS; ++c, data = caption_frame_read_char(frame, r, c, &styl, &unl)) {
            uint16_t cc_data = eia608_from_utf8_1(data, DEFAULT_CHANNEL);

            if (unl != prev_unl || styl != prev_styl) {
                encode_cc(sei, frame->timestamp, eia608_midrow_change(DEFAULT_CHANNEL, styl, unl), cc_count);
                prev_unl = unl, prev_styl = styl;
            }

            if (!cc_data) {
                // We do't want to write bad data, so just ignore it.
            } else if (eia608_is_basicna(prev_cc_data)) {
                if (eia608_is_basicna(cc_data)) {
                    // previous and current chars are both basicna, combine them into current
                    encode_cc(sei, frame->timestamp,  eia608_from_basicna(prev_cc_data, cc_data), cc_count);
                } else if (eia608_is_westeu(cc_data)) {
                    // extended charcters overwrite the previous charcter, so insert a dummy char thren write the extended char
                    encode_cc(sei, frame->timestamp,  eia608_from_basicna(prev_cc_data, eia608_from_utf8_1(EIA608_CHAR_SPACE, DEFAULT_CHANNEL)), cc_count);
                    encode_cc(sei, frame->timestamp, cc_data, cc_count);
                } else {
                    // previous was basic na, but current isnt; write previous and current
                    encode_cc(sei, frame->timestamp,  prev_cc_data, cc_count);
                    encode_cc(sei, frame->timestamp,  cc_data, cc_count);
                }

                prev_cc_data = 0; // previous is handled, we can forget it now
            } else if (eia608_is_westeu(cc_data)) {
                // extended chars overwrite the previous chars, so insert a dummy char
                // TODO create a map of alternamt chars for eia608_is_westeu instead of using space
                encode_cc(sei, frame->timestamp,  eia608_from_utf8_1(EIA608_CHAR_SPACE, DEFAULT_CHANNEL), cc_count);
                encode_cc(sei, frame->timestamp,  cc_data, cc_count);
            } else if (eia608_is_basicna(cc_data)) {
                prev_cc_data = cc_data;
            } else {
                encode_cc(sei, frame->timestamp, cc_data, cc_count);
            }

            if (eia608_is_specialna(cc_data)) {
                // specialna are treated as control charcters. Duplicated control charcters are discarded
                // So we write a resume after a specialna as a noop to break repetition detection
                // TODO only do this if the same charcter is repeated
                encode_cc(sei, frame->timestamp, eia608_control_command(eia608_control_resume_caption_loading, DEFAULT_CHANNEL), cc_count);
            }
        }

        if (0 != prev_cc_data) {
            encode_cc(sei, frame->timestamp, prev_cc_data, cc_count);
        }
    }

    sei->timestamp = frame->timestamp;
    //sei_dump(sei);

    return LIBCAPTION_OK;
}