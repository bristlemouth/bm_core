#ifndef __DFU_MESSAGE_STRUCTS_H__
#define __DFU_MESSAGE_STRUCTS_H__


#include <stdint.h>
#include <assert.h>
#include <stdlib.h>

typedef struct __attribute__((__packed__)) bm_dfu_img_info_s {
    uint32_t image_size;
    uint16_t chunk_size;
    uint16_t crc16;
    uint8_t major_ver;
    uint8_t minor_ver;
    uint32_t filter_key; // Will be used later for image rejection.
    uint32_t gitSHA;
} bm_dfu_img_info_t;

#define BM_DFU_IMG_INFO_FORCE_UPDATE (0x4CEDc0fe)

typedef struct __attribute__((__packed__)) bm_dfu_frame_header_s {
    uint8_t frame_type;
} BmDfuFrameHeader;

typedef struct __attribute__((__packed__)) bm_dfu_frame_s {
    BmDfuFrameHeader header;
    uint8_t payload[0];
} bm_dfu_frame_t;

typedef struct __attribute__((__packed__)) bm_dfu_event_init_success_s {
    uint8_t reserved;
}bm_dfu_event_init_success_t;

typedef struct __attribute__((__packed__)) bm_dfu_address_s {
    uint64_t src_node_id;
    uint64_t dst_node_id;
} BmDfuEventAddress;

typedef struct __attribute__((__packed__)) bm_dfu_event_chunk_request_s {
    BmDfuEventAddress addresses;
    uint16_t seq_num;
} BmDfuEventChunkRequest;

typedef struct __attribute__((__packed__)) bm_dfu_event_image_chunk_s {
    BmDfuEventAddress addresses;
    uint16_t payload_length;
    uint8_t payload_buf[0];
} BmDfuEventImageChunk;

typedef struct __attribute__((__packed__)) bm_dfu_result_s {
    BmDfuEventAddress addresses;
    uint8_t success;
    uint8_t err_code;
} BmDfuEventResult;

typedef struct __attribute__((__packed__)) bm_dfu_event_img_info_s {
    BmDfuEventAddress addresses;
    bm_dfu_img_info_t img_info;
} BmDfuEventImgInfo;

#ifdef __cplusplus
extern "C" {
#endif

#define DFU_HEADER_OFFSET_BYTES (0)
#define DFU_IMG_START_OFFSET_BYTES (sizeof(bm_dfu_img_info_t))
static_assert(DFU_IMG_START_OFFSET_BYTES > DFU_HEADER_OFFSET_BYTES, "Invalid DFU image offset");

#ifdef __cplusplus
}
#endif

#endif // __DFU_MESSAGE_STRUCTS_H__
