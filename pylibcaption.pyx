import logging

from libc.stdint cimport int8_t, uint8_t, uint16_t, uint32_t, uint64_t, int64_t
import numpy as np
from six.moves import range
cimport numpy as np
cimport c_pylibcaption as lib


cdef class CaptionFrame:
    cdef lib.caption_frame_t frame


def eia608_get_frame():
    f = CaptionFrame()
    lib.caption_frame_init(&f.frame)
    return f


def eia608_dump(CaptionFrame caption_frame, np.ndarray[np.uint8_t, ndim=1] cc_data_array, double timestamp):
    cdef int i
    cdef lib.scc_t* scc = NULL;
    cdef size_t data_size = cc_data_array.shape[0]
    cdef uint16_t world
    cdef uint8_t cc_valid
    cdef uint8_t cc_type

    cdef lib.libcaption_stauts_t status = lib.LIBCAPTION_OK

    for i in range(0, data_size, 3):
        cc_valid = (cc_data_array[i] & 4) >>2
        cc_type = cc_data_array[i] & 3
        # Not valid or taken into account
        if cc_valid != 1 or cc_type == 1:
            continue
        # Non data
        if ((cc_data_array[i] == 0xFA or cc_data_array[i] == 0xFC or cc_data_array[i] == 0xFD) and (
                cc_data_array[i + 1] & 0x7F) == 0 and (cc_data_array[i + 2] & 0x7F) == 0):
            continue
        world = ( cc_data_array[i + 1] << 8 ) | cc_data_array[i + 2]
        if lib.eia608_parity_varify(world) == 0:
            continue
        # lib.eia608_dump(world)
        # logging.info("Decode %d %d %d %d", cc_valid, cc_type, cc_data_array[i + 1] & 0x7f, cc_data_array[i + 2] & 0x7f)
        if cc_type == lib.cea708_cc_type_t.cc_type_ntsc_cc_field_1:
            status = lib.caption_frame_decode(&caption_frame.frame, world, timestamp)
            if status == lib.libcaption_stauts_t.LIBCAPTION_READY:
                lib.caption_frame_dump(&caption_frame.frame)


cdef get_messages_from_sei(lib.sei_t* sei, double timestamp):
    cdef lib.cea708_t cea708
    cdef lib.sei_message_t* msg
    cdef uint8_t* data
    cdef size_t size
    cdef np.ndarray msg_output
    output = []

    lib.cea708_init(&cea708, timestamp)
    msg = sei.head
    while msg:
        data = lib.sei_message_data(msg)
        size = lib.sei_message_size(msg)

        if lib.sei_message_type(msg) == lib.sei_type_user_data_registered_itu_t_t35:

            if lib.cea708_parse_h264(data, size, &cea708) != lib.LIBCAPTION_OK:
                logging.error("Failed to parse cea708")
            else:
                msg_output = np.zeros([cea708.user_data.cc_count * 3,], dtype=np.uint8)
                for i in range(cea708.user_data.cc_count):
                    msg_output[3*i] = (cea708.user_data.cc_data[i].marker_bits << 3) | (cea708.user_data.cc_data[i].cc_valid << 2) | cea708.user_data.cc_data[i].cc_type
                    msg_output[3*i+1] = cea708.user_data.cc_data[i].cc_data >> 8
                    msg_output[3*i+2] = cea708.user_data.cc_data[i].cc_data >> 0
                output.append(msg_output)
                lib.cea708_dump(&cea708)

        msg = lib.sei_message_next(msg)
    return output


def str_to_eia(np.ndarray[np.uint8_t, ndim=1] srt_data):

    cdef size_t data_size = srt_data.shape[0]
    cdef uint8_t* data = <uint8_t*> srt_data.data
    cdef lib.srt_cue_t* cue
    cdef lib.caption_frame_t frame
    cdef lib.srt_t* srt
    cdef lib.sei_t sei
    output = []

    srt = lib.srt_parse(data, data_size)

    cue = srt.cue_head
    while cue:
        lib.caption_frame_init(&frame)
        lib.srt_cue_to_caption_frame(cue, &frame)
        # lib.caption_frame_dump(&frame)

        # Convert to sei
        lib.sei_init(&sei, cue.timestamp)
        lib.sei_from_caption_frame(&sei, &frame)
        output.extend((get_messages_from_sei(&sei, cue.timestamp), cue.timestamp))
        # lib.sei_dump(&sei)
        cue = cue.next

    lib.srt_free(srt)
    return output
