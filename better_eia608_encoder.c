#include "better_eia608_encoder.h"
#include <stdio.h>
#include "mpeg.h"

#define PADDING_COMMAND 0x8080



static void sei_append_708(sei_t* sei, cea708_t* cea708)
{
    sei_message_t* msg = sei_message_new(sei_type_user_data_registered_itu_t_t35, 0, CEA608_MAX_SIZE);
    msg->size = cea708_render(cea708, sei_message_data(msg), sei_message_size(msg));
    sei_message_append(sei, msg);
    cea708_init(cea708, sei->timestamp); // will confgure using HLS compatiable defaults
}


static void sei_encode_eia608(sei_t* sei, cea708_t* cea708, uint16_t cc_data, cea708_cc_type_t type, int channel, int cc_count)
{
    // This one is full, flush and init a new one
    if (cc_count == cea708->user_data.cc_count) {
        sei_append_708(sei, cea708);
    }

    if (0 == cea708->user_data.cc_count) { // This is a new 708 header, but a continuation of a 608 stream
        cea708_add_cc_data(cea708, 1, type, eia608_control_command(eia608_control_resume_caption_loading, channel));
        cea708_add_cc_data(cea708, 1, type, eia608_control_command(eia608_control_resume_caption_loading, channel));
    }

    if (0 == cc_data) { // Finished
        sei_encode_eia608(sei, cea708, eia608_control_command(eia608_control_end_of_caption, channel), type, channel, cc_count);
        sei_encode_eia608(sei, cea708, eia608_control_command(eia608_control_end_of_caption, channel), type, channel, cc_count);
        sei_append_708(sei, cea708);
        return;
    }

    cea708_add_cc_data(cea708, 1, type, cc_data);
}


libcaption_stauts_t better_sei_for_padding(sei_t* sei, cea708_cc_type_t type, int channel, int cc_count)
{
    cea708_t cea708;
    cea708_init(&cea708, sei->timestamp);
    cea708_add_cc_data(&cea708, 1, type, PADDING_COMMAND);
    sei_append_708(sei, &cea708);
    return LIBCAPTION_OK;
}

libcaption_stauts_t better_sei_for_display(sei_t* sei, cea708_cc_type_t type, int channel, int cc_count)
{
    cea708_t cea708;
    cea708_init(&cea708, sei->timestamp);

    cea708_add_cc_data(&cea708, 1, type, eia608_control_command(eia608_control_erase_display_memory, channel));
    cea708_add_cc_data(&cea708, 1, type, eia608_control_command(eia608_control_end_of_caption, channel));

    sei_append_708(sei, &cea708);
    return LIBCAPTION_OK;
}

libcaption_stauts_t better_sei_for_clear(sei_t* sei, cea708_cc_type_t type, int channel, int cc_count)
{
    cea708_t cea708;
    cea708_init(&cea708, sei->timestamp);

    cea708_add_cc_data(&cea708, 1, type, eia608_control_command(eia608_control_erase_display_memory, channel));

    sei_append_708(sei, &cea708);
    return LIBCAPTION_OK;
}

////////////////////////////////////////////////////////////////////////////////
// logic of this function is the same as sei_from_caption_frame in mpeg.c
libcaption_stauts_t better_sei_for_captions(sei_t* sei, caption_frame_t* frame, cea708_cc_type_t type, int channel, int cc_count)
{

    int r, c;
    int unl, prev_unl;
    cea708_t cea708;
    const char* data;
    uint16_t prev_cc_data;
    eia608_style_t styl, prev_styl;

    sei_init(sei, frame->timestamp);
    cea708_init(&cea708, sei->timestamp);

    sei_encode_eia608(sei, &cea708, eia608_control_command(eia608_control_erase_non_displayed_memory, channel), type, channel, cc_count);
    sei_encode_eia608(sei, &cea708, eia608_control_command(eia608_control_resume_caption_loading, channel), type, channel, cc_count);

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
            sei_encode_eia608(sei, &cea708, eia608_row_column_pramble(r, c, channel, 0), type, channel, cc_count);
            if (tab) {
               sei_encode_eia608(sei, &cea708, eia608_tab(tab, channel), type, channel, cc_count);
            }
        } else {
            sei_encode_eia608(sei, &cea708, eia608_row_style_pramble(r, channel, styl, unl), type, channel, cc_count);
            prev_unl = unl, prev_styl = styl;
        }

        // Write the row
        for (prev_cc_data = 0, data = caption_frame_read_char(frame, r, c, 0, 0);
             (*data) && c < SCREEN_COLS; ++c, data = caption_frame_read_char(frame, r, c, &styl, &unl)) {
            uint16_t cc_data = eia608_from_utf8_1(data, channel);

            if (unl != prev_unl || styl != prev_styl) {
                sei_encode_eia608(sei, &cea708, eia608_midrow_change(channel, styl, unl), type, channel, cc_count);
                prev_unl = unl, prev_styl = styl;
            }

            if (!cc_data) {
                // We do't want to write bad data, so just ignore it.
            } else if (eia608_is_basicna(prev_cc_data)) {
                if (eia608_is_basicna(cc_data)) {
                    // previous and current chars are both basicna, combine them into current
                    sei_encode_eia608(sei, &cea708, eia608_from_basicna(prev_cc_data, cc_data), type, channel, cc_count);
                } else if (eia608_is_westeu(cc_data)) {
                    // extended charcters overwrite the previous charcter, so insert a dummy char thren write the extended char
                    sei_encode_eia608(sei, &cea708, eia608_from_basicna(prev_cc_data, eia608_from_utf8_1(EIA608_CHAR_SPACE, channel)), type, channel, cc_count);
                    sei_encode_eia608(sei, &cea708, cc_data, type, channel, cc_count);
                } else {
                    // previous was basic na, but current isnt; write previous and current
                    sei_encode_eia608(sei, &cea708, prev_cc_data, type, channel, cc_count);
                    sei_encode_eia608(sei, &cea708, cc_data, type, channel, cc_count);
                }

                prev_cc_data = 0; // previous is handled, we can forget it now
            } else if (eia608_is_westeu(cc_data)) {
                // extended chars overwrite the previous chars, so insert a dummy char
                // TODO create a map of alternamt chars for eia608_is_westeu instead of using space
                sei_encode_eia608(sei, &cea708, eia608_from_utf8_1(EIA608_CHAR_SPACE, channel), type, channel, cc_count);
                sei_encode_eia608(sei, &cea708, cc_data, type, channel, cc_count);
            } else if (eia608_is_basicna(cc_data)) {
                prev_cc_data = cc_data;
            } else {
                sei_encode_eia608(sei, &cea708, cc_data, type, channel, cc_count);
            }

            if (eia608_is_specialna(cc_data)) {
                // specialna are treated as control charcters. Duplicated control charcters are discarded
                // So we write a resume after a specialna as a noop to break repetition detection
                // TODO only do this if the same charcter is repeated
                sei_encode_eia608(sei, &cea708, eia608_control_command(eia608_control_resume_caption_loading, channel), type, channel, cc_count);
            }
        }

        if (0 != prev_cc_data) {
            sei_encode_eia608(sei, &cea708, prev_cc_data, type, channel, cc_count);
        }
    }

    sei_append_708(sei, &cea708);

    sei->timestamp = frame->timestamp;
    //sei_dump(sei);

    return LIBCAPTION_OK;
}

