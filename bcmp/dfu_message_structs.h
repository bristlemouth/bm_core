#ifndef __DFU_MESSAGE_STRUCTS_H__
#define __DFU_MESSAGE_STRUCTS_H__


#include <stdint.h>
#include <assert.h>
#include <stdlib.h>

typedef struct __attribute__((__packed__)) BmDfuImgInfo {
    uint32_t image_size;
    uint16_t chunk_size;
    uint16_t crc16;
    uint8_t major_ver;
    uint8_t minor_ver;
    uint32_t filter_key; // Will be used later for image rejection.
    uint32_t gitSHA;
} BmDfuImgInfo;

#define BM_DFU_IMG_INFO_FORCE_UPDATE (0x4CEDc0fe)

typedef struct __attribute__((__packed__)) BmDfuFrameHeader {
    uint8_t frame_type;
} BmDfuFrameHeader;

typedef struct __attribute__((__packed__)) BmDfuFrame {
    BmDfuFrameHeader header;
    uint8_t payload[0];
} BmDfuFrame;

typedef struct __attribute__((__packed__)) BmDfuEventInitSuccess {
    uint8_t reserved;
} BmDfuEventInitSuccess;

typedef struct __attribute__((__packed__)) BmDfuEventAddress {
    uint64_t src_node_id;
    uint64_t dst_node_id;
} BmDfuEventAddress;

typedef struct __attribute__((__packed__)) BmDfuEventChunkRequest {
    BmDfuEventAddress addresses;
    uint16_t seq_num;
} BmDfuEventChunkRequest;

typedef struct __attribute__((__packed__)) BmDfuEventImageChunk {
    BmDfuEventAddress addresses;
    uint16_t payload_length;
    uint8_t payload_buf[0];
} BmDfuEventImageChunk;

typedef struct __attribute__((__packed__)) BmDfuEventResult {
    BmDfuEventAddress addresses;
    uint8_t success;
    uint8_t err_code;
} BmDfuEventResult;

typedef struct __attribute__((__packed__)) BmDfuEventImgInfo {
    BmDfuEventAddress addresses;
    BmDfuImgInfo img_info;
} BmDfuEventImgInfo;

#ifdef __cplusplus
extern "C" {
#endif

#define DFU_HEADER_OFFSET_BYTES (0)
#define DFU_IMG_START_OFFSET_BYTES (sizeof(BmDfuImgInfo))
static_assert(DFU_IMG_START_OFFSET_BYTES > DFU_HEADER_OFFSET_BYTES, "Invalid DFU image offset");

#ifdef __cplusplus
}
#endif

#endif // __DFU_MESSAGE_STRUCTS_H__
