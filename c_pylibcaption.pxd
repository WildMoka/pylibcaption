import logging

from libc.stdint cimport int8_t, uint8_t, uint16_t, uint32_t, uint64_t, int64_t
import numpy as np
cimport numpy as np

cdef extern from "utf8.h":
    ctypedef char utf8_char_t;

cdef extern from "xds.h":
    ctypedef struct xds_t:
        int state;
        uint8_t class_code
        uint8_t type
        uint32_t size
        uint8_t content[32]
        uint8_t checksum

    void xds_init(xds_t*xds);
    int xds_decode(xds_t*xds, uint16_t cc);

cdef extern from "caption.h":
    ctypedef enum libcaption_stauts_t:
        LIBCAPTION_ERROR = 0
        LIBCAPTION_OK
        LIBCAPTION_READY

    ctypedef struct caption_frame_cell_t:
        unsigned int uln
        unsigned int sty
        char data[5]

    ctypedef struct caption_frame_buffer_t:
        caption_frame_cell_t cell[15][32]

    ctypedef struct caption_frame_state_t:
        unsigned int uln
        unsigned int sty
        unsigned int rup
        int8_t row, col;
        uint16_t cc_data;

    ctypedef struct caption_frame_t:
        double timestamp;
        xds_t xds;
        caption_frame_state_t state;
        caption_frame_buffer_t front;
        caption_frame_buffer_t back;
        caption_frame_buffer_t*write;
        libcaption_stauts_t status;

    cdef void caption_frame_init(caption_frame_t*frame);
    libcaption_stauts_t caption_frame_decode(caption_frame_t*frame, uint16_t cc_data, double timestamp);
    void caption_frame_dump(caption_frame_t*frame);
    int caption_frame_from_text(caption_frame_t * frame, const utf8_char_t * data);

cdef extern from "scc.h":
    ctypedef struct scc_t:
        double timestamp
        unsigned int cc_aloc
        unsigned int cc_size
        uint16_t*cc_data

    scc_t*scc_new(int cc_count);
    scc_t*scc_free(scc_t*scc);

    size_t scc_to_608(scc_t** scc, const char*data);

cdef extern from "eia608.h":
    int eia608_parity_varify(uint16_t cc_data)
    void eia608_dump(uint16_t cc_data);


cdef extern from "mpeg.h":
    ctypedef enum sei_msgtype_t:
        sei_type_buffering_period
        sei_type_pic_timing
        sei_type_pan_scan_rect
        sei_type_filler_payload
        sei_type_user_data_registered_itu_t_t35
        sei_type_user_data_unregistered
        sei_type_recovery_point
        sei_type_dec_ref_pic_marking_repetition
        sei_type_spare_pic
        sei_type_scene_info
        sei_type_sub_seq_info
        sei_type_sub_seq_layer_characteristics
        sei_type_sub_seq_characteristics
        sei_type_full_frame_freeze
        sei_type_full_frame_freeze_release
        sei_type_full_frame_snapshot
        sei_type_progressive_refinement_segment_start
        sei_type_progressive_refinement_segment_end
        sei_type_motion_constrained_slice_group_set
        sei_type_film_grain_characteristics
        sei_type_deblocking_filter_display_preference
        sei_type_stereo_video_info

    ctypedef struct sei_message_t:
        size_t size;
        sei_msgtype_t type
        sei_message_t*next

    ctypedef struct sei_t:
        double timestamp
        sei_message_t*head
        sei_message_t*tail

    void sei_init(sei_t*sei, double timestamp);
    libcaption_stauts_t sei_parse(sei_t*sei, const uint8_t*data, size_t size, double timestamp);
    libcaption_stauts_t sei_from_caption_frame(sei_t* sei, caption_frame_t* frame);
    libcaption_stauts_t sei_from_caption_clear(sei_t * sei);
    sei_message_t* sei_message_next(sei_message_t* msg);
    uint8_t* sei_message_data(sei_message_t* msg);
    size_t sei_message_size(sei_message_t* msg);
    sei_msgtype_t sei_message_type(sei_message_t* msg);
    void sei_dump(sei_t*sei);
    void sei_free(sei_t*sei);

cdef extern from "cea708.h":
    ctypedef enum cea708_cc_type_t:
        cc_type_ntsc_cc_field_1 = 0,
        cc_type_ntsc_cc_field_2 = 1,
        cc_type_dtvcc_packet_data = 2,
        cc_type_dtvcc_packet_start = 3,

    ctypedef struct cc_data_t:
        unsigned int marker_bits
        unsigned int cc_valid
        unsigned int cc_type
        unsigned int cc_data

    ctypedef struct user_data_t:
        unsigned int process_em_data_fla
        unsigned int process_cc_data_flag
        unsigned int additional_data_flag
        unsigned int cc_count
        unsigned int em_data
        cc_data_t cc_data[32];

    ctypedef enum itu_t_t35_country_code_t:
        country_united_states

    ctypedef enum itu_t_t35_provider_code_t:
        t35_provider_direct_tv
        t35_provider_atsc

    ctypedef struct cea708_t:
        itu_t_t35_country_code_t country;
        itu_t_t35_provider_code_t provider;
        uint32_t user_identifier;
        uint8_t user_data_type_code;
        uint8_t directv_user_data_length;
        user_data_t user_data;
        double timestamp

    int cea708_init(cea708_t*cea708, double timestamp)
    void cea708_dump(cea708_t*cea708)
    libcaption_stauts_t cea708_parse_h264(const uint8_t*data, size_t size, cea708_t*cea708)

cdef extern from "vtt.h":
    ctypedef enum VTT_BLOCK_TYPE:
        VTT_REGION
        VTT_STYLE
        VTT_NOTE
        VTT_CUE

    ctypedef struct vtt_block_t:
        vtt_block_t* next
        VTT_BLOCK_TYPE type
        double timestamp
        double duration
        char* cue_settings
        char* cue_id
        size_t text_size
        char* block_text

    ctypedef struct vtt_t:
        vtt_block_t* region_tail
        vtt_block_t* region_head
        vtt_block_t* style_head
        vtt_block_t* style_tail
        vtt_block_t* cue_head
        vtt_block_t* cue_tail

    ctypedef vtt_t srt_t
    srt_t* srt_parse(const uint8_t*data, size_t size);
    void srt_free(srt_t* srt);

cdef extern from "srt.h":
    ctypedef vtt_t srt_t
    ctypedef vtt_block_t srt_cue_t
    srt_t* srt_parse(const uint8_t* data, size_t size);
    int srt_cue_to_caption_frame(srt_cue_t* cue, caption_frame_t* frame)

cdef extern from "eia608_encoder.h":
    libcaption_stauts_t sei_for_display(sei_t * sei, int cc_count);
    libcaption_stauts_t sei_for_padding(sei_t * sei, int cc_count);
    libcaption_stauts_t sei_for_clear(sei_t * sei, int cc_count);
    libcaption_stauts_t sei_for_captions(sei_t * sei, caption_frame_t * frame, int cc_count);