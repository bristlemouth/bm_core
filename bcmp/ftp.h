#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "bm_os.h"
#include "messages.h"

#ifndef BM_FTP_EVENT_TASK_PRIORITY
#define BM_FTP_EVENT_TASK_PRIORITY 11
#endif

typedef enum {
  BmFtpEventStart = 0,
  BmFtpEventFetch,
  BmFtpEventAck,
  BmFtpEventChunkRequest,
  BmFtpEventChunk,
  BmFtpEventEnd,
  BmFtpEventAbort,
} BmFtpEventType;

typedef struct {
  BmFtpEventType type;
  uint32_t seq_num;
  uint8_t *payload;
  size_t payload_len;
} BmFtpEvent;

typedef BmErr (*BmFtpEventHandler)(const BmFtpEvent *event, void *context);

BmErr bm_ftp_init(void);
BmErr bm_ftp_coordinator_init(void);
BmQueue bm_ftp_get_event_queue(void);
void bm_ftp_set_event_handler(BmFtpEventHandler handler, void *context);

BmErr bm_ftp_start_fetch(uint64_t source_node_id, uint32_t transfer_id,
                         BmFtpEndpointKind source_kind,
                         const uint8_t *source_spec, uint16_t source_spec_len,
                         BmFtpEndpointKind sink_kind, const uint8_t *sink_spec,
                         uint16_t sink_spec_len);
BmErr bm_ftp_send_ack(uint64_t dst_node_id, uint32_t transfer_id, bool success,
                      BmFtpErr error, uint32_t total_size, uint16_t crc16,
                      uint16_t chunk_size, uint32_t seq_num);
BmErr bm_ftp_send_start(uint64_t dst_node_id, uint32_t transfer_id,
                        uint32_t total_size, uint16_t requested_chunk_size,
                        uint16_t crc16, BmFtpEndpointKind sink_kind,
                        const uint8_t *sink_spec, uint16_t sink_spec_len);
BmErr bm_ftp_send_end(uint64_t dst_node_id, uint32_t transfer_id, bool success,
                      BmFtpErr error, uint32_t bytes_received,
                      uint16_t running_crc16);
BmErr bm_ftp_send_fetch(uint64_t dst_node_id, uint32_t transfer_id,
                        BmFtpEndpointKind source_kind,
                        const uint8_t *source_spec, uint16_t source_spec_len);
BmErr bm_ftp_request_chunk(uint64_t dst_node_id, uint32_t transfer_id,
                           uint32_t offset, uint16_t length);
BmErr bm_ftp_send_chunk(uint64_t dst_node_id, uint32_t transfer_id,
                        uint32_t offset, const uint8_t *payload,
                        uint16_t payload_len, uint32_t seq_num);
BmErr bm_ftp_send_abort(uint64_t dst_node_id, uint32_t transfer_id,
                        BmFtpErr error);

#ifdef __cplusplus
}
#endif
