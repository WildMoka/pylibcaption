#ifndef EIA608_ENCODER_H
#define EIA608_ENCODER_H

#include "mpeg.h"

/*
 * Encodes the cc elements in parameter in the cea708.
 * Number of cc elements in a cea708 is decided by framerate of the video.
 * This function takes care of encoding the right number of elements
 */
libcaption_stauts_t encode_cc(sei_t* sei, double timestamp, uint16_t cc_element, int cc_count);

/*
 * Appends a cea708 to a sei
 * It is a duplicate of sei_append_708 in mpeg.c
 */
void sei_append(sei_t* sei, cea708_t* cea708);

/*
 * Encode the end of caption command
 */
libcaption_stauts_t sei_for_display(sei_t * sei, int cc_count);

/*
 * Encodes the padding command 0x8080
 */
libcaption_stauts_t sei_for_padding(sei_t * sei, int cc_count);

/*
 * Encodes erase_display_memory command
 */
libcaption_stauts_t sei_for_clear(sei_t * sei, int cc_count);

/*
 * Encodes the captions contained in the frame given as parameter.
 * the sei will contain as many cea708 elements as necessary to encode the captions.
 */
libcaption_stauts_t sei_for_captions(sei_t* sei, caption_frame_t* frame, int cc_count);

#endif