#ifndef BETTER_EIA608_ENCODER_H
#define BETTER_EIA608_ENCODER_H

#include "mpeg.h"

/*
 * Encode the erase_display_memory and end of caption commands
 */
libcaption_stauts_t better_sei_for_display(sei_t* sei, cea708_cc_type_t type, int channel, int cc_count);

/*
 * Encodes the padding command 0x8080
 */
libcaption_stauts_t better_sei_for_padding(sei_t * sei, cea708_cc_type_t type, int channel, int cc_count);

/*
 * Encodes erase_display_memory command
 */
libcaption_stauts_t better_sei_for_clear(sei_t * sei, cea708_cc_type_t type, int channel, int cc_count);

/*
 * Encodes the captions contained in the frame given as parameter.
 * the sei will contain as many cea708 elements as necessary to encode the captions.
 */
libcaption_stauts_t better_sei_for_captions(sei_t* sei, caption_frame_t* frame, cea708_cc_type_t type, int channel, int cc_count);

#endif