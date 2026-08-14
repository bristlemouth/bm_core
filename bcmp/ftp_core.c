#include "ftp.h"

#include "bcmp.h"
#include "bm_config.h"
#include "bm_os.h"
#include "device.h"
#include "ftp_message_structs.h"
#include "messages.h"
#include "packet.h"
#include <string.h>

#define bm_ftp_event_queue_len (8U)

static BmQueue ftp_event_queue;
static BmFtpEventHandler ftp_event_handler;
static void *ftp_event_handler_context;

static uint32_t bm_ftp_event_transfer_id(const BmFtpEvent *event) {
  uint32_t transfer_id;
  memcpy(&transfer_id, event->payload + sizeof(BmFtpAddress),
         sizeof(transfer_id));
  return transfer_id;
}

static BmFtpEventType bm_ftp_event_type(BcmpMessageType type) {
  switch (type) {
  case BcmpFTPStartMessage:
    return BmFtpEventStart;
  case BcmpFTPFetchMessage:
    return BmFtpEventFetch;
  case BcmpFTPAckMessage:
    return BmFtpEventAck;
  case BcmpFTPChunkReqMessage:
    return BmFtpEventChunkRequest;
  case BcmpFTPChunkMessage:
    return BmFtpEventChunk;
  case BcmpFTPEndMessage:
    return BmFtpEventEnd;
  case BcmpFTPAbortMessage:
  default:
    return BmFtpEventAbort;
  }
}

static void bm_ftp_event_thread(void *parameters) {
  (void)parameters;

  for (;;) {
    BmFtpEvent event;
    if (bm_queue_receive(ftp_event_queue, &event, UINT32_MAX) == BmOK) {
      if (ftp_event_handler) {
        ftp_event_handler(&event, ftp_event_handler_context);
      } else if (event.type == BmFtpEventStart ||
                 event.type == BmFtpEventFetch) {
        BmFtpAddress *address = (BmFtpAddress *)event.payload;
        bm_ftp_send_ack(address->src_node_id, bm_ftp_event_transfer_id(&event),
                        false, BmFtpErrUnsupported, 0, 0, 0);
      }
      bm_free(event.payload);
    }
  }
}

static bool bm_ftp_valid_message(BcmpMessageType type, const uint8_t *payload,
                                 size_t payload_len) {
  if (!payload) {
    return false;
  }

  switch (type) {
  case BcmpFTPStartMessage: {
    if (payload_len < sizeof(BmFtpStart)) {
      return false;
    }
    const BmFtpStart *start = (const BmFtpStart *)payload;
    return start->sink_spec_len == payload_len - sizeof(BmFtpStart);
  }
  case BcmpFTPFetchMessage: {
    if (payload_len < sizeof(BmFtpFetch)) {
      return false;
    }
    const BmFtpFetch *fetch = (const BmFtpFetch *)payload;
    return fetch->source_spec_len == payload_len - sizeof(BmFtpFetch);
  }
  case BcmpFTPAckMessage: {
    return payload_len == sizeof(BmFtpAck);
  }
  case BcmpFTPChunkReqMessage: {
    return payload_len == sizeof(BmFtpChunkRequest);
  }
  case BcmpFTPChunkMessage: {
    if (payload_len < sizeof(BmFtpChunk)) {
      return false;
    }
    const BmFtpChunk *chunk = (const BmFtpChunk *)payload;
    return chunk->payload_length == payload_len - sizeof(BmFtpChunk);
  }
  case BcmpFTPEndMessage: {
    return payload_len == sizeof(BmFtpEnd);
  }
  case BcmpFTPAbortMessage: {
    return payload_len == sizeof(BmFtpAbort);
  }
  default: {
    return false;
  }
  }
}

static BmErr bm_ftp_process_message(BcmpProcessData data) {
  BcmpMessageType type = (BcmpMessageType)data.header->type;
  if (!bm_ftp_valid_message(type, data.payload, data.size)) {
    return BmEBADMSG;
  }

  BmFtpAddress *address = (BmFtpAddress *)data.payload;
  if (address->dst_node_id != node_id()) {
    return BmENOTINTREC;
  }

  uint8_t *payload = bm_malloc(data.size);
  if (!payload) {
    return BmENOMEM;
  }
  memcpy(payload, data.payload, data.size);

  BmFtpEvent event = {
      .type = bm_ftp_event_type(type),
      .payload = payload,
      .payload_len = data.size,
  };
  if (bm_queue_send(ftp_event_queue, &event, 0) != BmOK) {
    bm_free(payload);
    return BmEAGAIN;
  }
  return BmOK;
}

BmErr bm_ftp_init(void) {
  BmErr err = BmOK;
  BcmpPacketCfg packet = {
      .sequenced_reply = false,
      .sequenced_request = false,
      .process = bm_ftp_process_message,
  };

  ftp_event_queue = bm_queue_create(bm_ftp_event_queue_len, sizeof(BmFtpEvent));
  if (!ftp_event_queue) {
    return BmENOMEM;
  }
  bm_err_check(err, bm_ftp_coordinator_init());
  bm_err_check(err, bm_task_create(bm_ftp_event_thread, "FTP Event", 1024, NULL,
                                   BM_FTP_EVENT_TASK_PRIORITY, NULL));
  bm_err_check(err, packet_add(&packet, BcmpFTPStartMessage));
  bm_err_check(err, packet_add(&packet, BcmpFTPAckMessage));
  bm_err_check(err, packet_add(&packet, BcmpFTPChunkReqMessage));
  bm_err_check(err, packet_add(&packet, BcmpFTPChunkMessage));
  bm_err_check(err, packet_add(&packet, BcmpFTPEndMessage));
  bm_err_check(err, packet_add(&packet, BcmpFTPAbortMessage));
  bm_err_check(err, packet_add(&packet, BcmpFTPFetchMessage));
  return err;
}

BmQueue bm_ftp_get_event_queue(void) { return ftp_event_queue; }

void bm_ftp_set_event_handler(BmFtpEventHandler handler, void *context) {
  ftp_event_handler = handler;
  ftp_event_handler_context = context;
}

BmErr bm_ftp_send_ack(uint64_t dst_node_id, uint32_t transfer_id, bool success,
                      BmFtpErr error, uint32_t total_size, uint16_t crc16,
                      uint16_t chunk_size) {
  BmFtpAck ack = {
      .addresses = {.src_node_id = node_id(), .dst_node_id = dst_node_id},
      .transfer_id = transfer_id,
      .total_size = total_size,
      .crc16 = crc16,
      .chunk_size = chunk_size,
      .success = success,
      .err_code = error,
  };

  return bcmp_tx(&multicast_global_addr, BcmpFTPAckMessage, (uint8_t *)&ack,
                 sizeof(ack), 0, NULL);
}

BmErr bm_ftp_send_start(uint64_t dst_node_id, uint32_t transfer_id, uint32_t total_size,
                        uint16_t requested_chunk_size, uint16_t crc16,
                        BmFtpEndpointKind sink_kind, const uint8_t *sink_spec,
                        uint16_t sink_spec_len) {
  if (sink_spec_len && !sink_spec) {
    return BmEINVAL;
  }

  size_t message_len = sizeof(BmFtpStart) + sink_spec_len;
  BmFtpStart *start = bm_malloc(message_len);
  if (!start) {
    return BmENOMEM;
  }
  start->addresses.src_node_id = node_id();
  start->addresses.dst_node_id = dst_node_id;
  start->transfer_id = transfer_id;
  start->total_size = total_size;
  start->requested_chunk_size = requested_chunk_size;
  start->crc16 = crc16;
  start->sink_kind = sink_kind;
  start->flags = 0;
  start->sink_spec_len = sink_spec_len;
  if (sink_spec_len) {
    memcpy(start->sink_spec, sink_spec, sink_spec_len);
  }
  BmErr err = bcmp_tx(&multicast_global_addr, BcmpFTPStartMessage, (uint8_t *)start,
                      message_len, 0, NULL);
  bm_free(start);
  return err;
}

BmErr bm_ftp_send_fetch(uint64_t dst_node_id, uint32_t transfer_id,
                        BmFtpEndpointKind source_kind, const uint8_t *source_spec,
                        uint16_t source_spec_len) {
  if (source_spec_len && !source_spec) {
    return BmEINVAL;
  }

  size_t message_len = sizeof(BmFtpFetch) + source_spec_len;
  BmFtpFetch *fetch = bm_malloc(message_len);
  if (!fetch) {
    return BmENOMEM;
  }
  fetch->addresses.src_node_id = node_id();
  fetch->addresses.dst_node_id = dst_node_id;
  fetch->transfer_id = transfer_id;
  fetch->source_kind = source_kind;
  fetch->flags = 0;
  fetch->source_spec_len = source_spec_len;
  if (source_spec_len) {
    memcpy(fetch->source_spec, source_spec, source_spec_len);
  }
  BmErr err = bcmp_tx(&multicast_global_addr, BcmpFTPFetchMessage, (uint8_t *)fetch,
                      message_len, 0, NULL);
  bm_free(fetch);
  return err;
}

BmErr bm_ftp_request_chunk(uint64_t dst_node_id, uint32_t transfer_id, uint32_t offset,
                           uint16_t length) {
  BmFtpChunkRequest request = {
      .addresses = {.src_node_id = node_id(), .dst_node_id = dst_node_id},
      .transfer_id = transfer_id,
      .offset = offset,
      .length = length,
      .reserved = 0,
  };
  return bcmp_tx(&multicast_global_addr, BcmpFTPChunkReqMessage, (uint8_t *)&request,
                 sizeof(request), 0, NULL);
}

BmErr bm_ftp_send_chunk(uint64_t dst_node_id, uint32_t transfer_id, uint32_t offset,
                        const uint8_t *payload, uint16_t payload_len) {
  if (payload_len && !payload) {
    return BmEINVAL;
  }

  size_t message_len = sizeof(BmFtpChunk) + payload_len;
  BmFtpChunk *chunk = bm_malloc(message_len);
  if (!chunk) {
    return BmENOMEM;
  }
  chunk->addresses.src_node_id = node_id();
  chunk->addresses.dst_node_id = dst_node_id;
  chunk->transfer_id = transfer_id;
  chunk->offset = offset;
  chunk->payload_length = payload_len;
  chunk->reserved = 0;
  if (payload_len) {
    memcpy(chunk->payload, payload, payload_len);
  }
  BmErr err = bcmp_tx(&multicast_global_addr, BcmpFTPChunkMessage, (uint8_t *)chunk,
                      message_len, 0, NULL);
  bm_free(chunk);
  return err;
}

BmErr bm_ftp_send_end(uint64_t dst_node_id, uint32_t transfer_id, bool success,
                      BmFtpErr error, uint32_t bytes_received, uint16_t running_crc16) {
  BmFtpEnd end = {
      .addresses = {.src_node_id = node_id(), .dst_node_id = dst_node_id},
      .transfer_id = transfer_id,
      .success = success,
      .err_code = error,
      .reserved = 0,
      .bytes_received = bytes_received,
      .running_crc16 = running_crc16,
      .reserved2 = 0,
  };
  return bcmp_tx(&multicast_global_addr, BcmpFTPEndMessage, (uint8_t *)&end, sizeof(end), 0,
                 NULL);
}

BmErr bm_ftp_send_abort(uint64_t dst_node_id, uint32_t transfer_id, BmFtpErr error) {
  BmFtpAbort abort = {
      .addresses = {.src_node_id = node_id(), .dst_node_id = dst_node_id},
      .transfer_id = transfer_id,
      .err_code = error,
      .reserved = {0},
  };
  return bcmp_tx(&multicast_global_addr, BcmpFTPAbortMessage, (uint8_t *)&abort,
                 sizeof(abort), 0, NULL);
}
