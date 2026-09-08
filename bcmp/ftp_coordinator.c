#include "ftp.h"

#include "device.h"
#include "ftp_endpoint.h"
#include <string.h>

#define bm_ftp_max_chunk_size (1024U)
#define bm_ftp_max_endpoint_spec_len (128U)

typedef enum {
  BmFtpCoordinatorIdle = 0,
  BmFtpCoordinatorWaitingAck,
  BmFtpCoordinatorServing,
  BmFtpCoordinatorReceiving,
} BmFtpCoordinatorState;

typedef struct {
  BmFtpCoordinatorState state;
  uint64_t peer_node_id;
  uint32_t transfer_id;
  uint32_t total_size;
  uint32_t bytes_received;
  uint16_t chunk_size;
  uint16_t crc16;
  BmFtpEndpointKind pending_sink_kind;
  uint16_t pending_sink_spec_len;
  uint8_t pending_sink_spec[bm_ftp_max_endpoint_spec_len];
  BmFtpSource source;
  BmFtpSink sink;
} BmFtpCoordinator;

static BmFtpCoordinator coordinator;

static BmFtpErr bm_ftp_error_from_bm_error(BmErr error) {
  if (error == BmENODEV) {
    return BmFtpErrUnsupported;
  }
  if (error == BmEINVAL) {
    return BmFtpErrInvalidSpec;
  }
  return BmFtpErrWriteFailed;
}

static void bm_ftp_reset(void) {
  if (coordinator.state == BmFtpCoordinatorServing) {
    bm_ftp_source_close(&coordinator.source);
  } else if (coordinator.state == BmFtpCoordinatorReceiving) {
    bm_ftp_sink_close(&coordinator.sink);
  }
  memset(&coordinator, 0, sizeof(coordinator));
}

static BmErr bm_ftp_reject(const BmFtpAddress *address, uint32_t transfer_id,
                           BmFtpErr error, uint32_t seq_num) {
  return bm_ftp_send_ack(address->src_node_id, transfer_id, false, error, 0, 0,
                         0, seq_num);
}

static BmErr bm_ftp_handle_start(const BmFtpEvent *event) {
  const BmFtpStart *start = (const BmFtpStart *)event->payload;
  uint32_t seq_num = event->seq_num;

  if (coordinator.state != BmFtpCoordinatorIdle) {
    return bm_ftp_reject(&start->addresses, start->transfer_id, BmFtpErrBusy,
                         seq_num);
  }
  if (start->total_size == 0 || start->requested_chunk_size == 0 ||
      start->requested_chunk_size > bm_ftp_max_chunk_size) {
    return bm_ftp_reject(&start->addresses, start->transfer_id,
                         BmFtpErrTooLarge, seq_num);
  }

  BmErr error = bm_ftp_sink_open((BmFtpEndpointKind)start->sink_kind,
                                 start->sink_spec, start->sink_spec_len,
                                 start->total_size, &coordinator.sink);
  if (error != BmOK) {
    return bm_ftp_reject(&start->addresses, start->transfer_id,
                         bm_ftp_error_from_bm_error(error), seq_num);
  }

  coordinator.state = BmFtpCoordinatorReceiving;
  coordinator.peer_node_id = start->addresses.src_node_id;
  coordinator.transfer_id = start->transfer_id;
  coordinator.total_size = start->total_size;
  coordinator.chunk_size = start->requested_chunk_size;
  coordinator.crc16 = start->crc16;

  error = bm_ftp_send_ack(coordinator.peer_node_id, coordinator.transfer_id,
                          true, BmFtpErrNone, coordinator.total_size,
                          coordinator.crc16, coordinator.chunk_size, seq_num);
  if (error != BmOK) {
    bm_ftp_reset();
    return error;
  }
  uint16_t requested_length = coordinator.total_size < coordinator.chunk_size
                                  ? (uint16_t)coordinator.total_size
                                  : coordinator.chunk_size;
  return bm_ftp_request_chunk(coordinator.peer_node_id, coordinator.transfer_id,
                              0, requested_length);
}

static BmErr bm_ftp_handle_fetch(const BmFtpEvent *event) {
  const BmFtpFetch *fetch = (const BmFtpFetch *)event->payload;
  uint32_t seq_num = event->seq_num;

  if (coordinator.state != BmFtpCoordinatorIdle) {
    return bm_ftp_reject(&fetch->addresses, fetch->transfer_id, BmFtpErrBusy,
                         seq_num);
  }

  BmErr error = bm_ftp_source_open((BmFtpEndpointKind)fetch->source_kind,
                                   fetch->source_spec, fetch->source_spec_len,
                                   &coordinator.source);
  if (error != BmOK) {
    return bm_ftp_reject(&fetch->addresses, fetch->transfer_id,
                         bm_ftp_error_from_bm_error(error), seq_num);
  }

  coordinator.state = BmFtpCoordinatorServing;
  coordinator.peer_node_id = fetch->addresses.src_node_id;
  coordinator.transfer_id = fetch->transfer_id;
  coordinator.total_size = coordinator.source.total_size;
  coordinator.crc16 = coordinator.source.crc16;
  coordinator.chunk_size = bm_ftp_max_chunk_size;

  if (coordinator.total_size == 0) {
    bm_ftp_reset();
    return bm_ftp_reject(&fetch->addresses, fetch->transfer_id,
                         BmFtpErrInvalidSpec, seq_num);
  }

  error = bm_ftp_send_ack(coordinator.peer_node_id, coordinator.transfer_id,
                          true, BmFtpErrNone, coordinator.total_size,
                          coordinator.crc16, coordinator.chunk_size, seq_num);
  if (error != BmOK) {
    bm_ftp_reset();
  }
  return error;
}

static BmErr bm_ftp_handle_ack(const BmFtpEvent *event) {
  const BmFtpAck *ack = (const BmFtpAck *)event->payload;
  if (coordinator.state != BmFtpCoordinatorWaitingAck ||
      ack->addresses.src_node_id != coordinator.peer_node_id ||
      ack->transfer_id != coordinator.transfer_id) {
    return BmEBADMSG;
  }
  if (!ack->success) {
    bm_ftp_reset();
    return BmECONNREFUSED;
  }
  if (ack->total_size == 0 || ack->chunk_size == 0 ||
      ack->chunk_size > bm_ftp_max_chunk_size) {
    bm_ftp_reset();
    return BmEBADMSG;
  }

  BmErr error = bm_ftp_sink_open(
      coordinator.pending_sink_kind, coordinator.pending_sink_spec,
      coordinator.pending_sink_spec_len, ack->total_size, &coordinator.sink);
  if (error != BmOK) {
    bm_ftp_send_abort(coordinator.peer_node_id, coordinator.transfer_id,
                      bm_ftp_error_from_bm_error(error));
    bm_ftp_reset();
    return error;
  }
  coordinator.state = BmFtpCoordinatorReceiving;
  coordinator.total_size = ack->total_size;
  coordinator.crc16 = ack->crc16;
  coordinator.chunk_size = ack->chunk_size;
  uint16_t requested_length = coordinator.total_size < coordinator.chunk_size
                                  ? (uint16_t)coordinator.total_size
                                  : coordinator.chunk_size;
  return bm_ftp_request_chunk(coordinator.peer_node_id, coordinator.transfer_id,
                              0, requested_length);
}

static BmErr bm_ftp_handle_chunk_request(const BmFtpEvent *event) {
  const BmFtpChunkRequest *request = (const BmFtpChunkRequest *)event->payload;
  uint32_t seq_num = event->seq_num;

  if (coordinator.state != BmFtpCoordinatorServing ||
      request->addresses.src_node_id != coordinator.peer_node_id ||
      request->transfer_id != coordinator.transfer_id || request->length == 0 ||
      request->length > coordinator.chunk_size ||
      request->offset >= coordinator.total_size ||
      request->length > coordinator.total_size - request->offset) {
    return BmEBADMSG;
  }

  uint8_t buffer[bm_ftp_max_chunk_size];
  BmErr error = bm_ftp_source_read_at(&coordinator.source, request->offset,
                                      buffer, request->length);
  if (error != BmOK) {
    bm_ftp_send_abort(coordinator.peer_node_id, coordinator.transfer_id,
                      bm_ftp_error_from_bm_error(error));
    bm_ftp_reset();
    return error;
  }
  return bm_ftp_send_chunk(coordinator.peer_node_id, coordinator.transfer_id,
                           request->offset, buffer, request->length, seq_num);
}

static BmErr bm_ftp_handle_chunk(const BmFtpEvent *event) {
  const BmFtpChunk *chunk = (const BmFtpChunk *)event->payload;
  if (coordinator.state != BmFtpCoordinatorReceiving ||
      chunk->addresses.src_node_id != coordinator.peer_node_id ||
      chunk->transfer_id != coordinator.transfer_id ||
      chunk->offset != coordinator.bytes_received ||
      chunk->payload_length == 0 ||
      chunk->payload_length > coordinator.chunk_size ||
      chunk->payload_length >
          coordinator.total_size - coordinator.bytes_received) {
    return BmEBADMSG;
  }

  BmErr error = bm_ftp_sink_write_at(&coordinator.sink, chunk->offset,
                                     chunk->payload, chunk->payload_length);
  if (error != BmOK) {
    bm_ftp_send_abort(coordinator.peer_node_id, coordinator.transfer_id,
                      bm_ftp_error_from_bm_error(error));
    bm_ftp_sink_abort(&coordinator.sink);
    bm_ftp_reset();
    return error;
  }
  coordinator.bytes_received += chunk->payload_length;
  if (coordinator.bytes_received == coordinator.total_size) {
    error = bm_ftp_sink_finalize(&coordinator.sink, coordinator.total_size,
                                 coordinator.crc16);
    BmFtpErr ftp_error =
        error == BmOK ? BmFtpErrNone : bm_ftp_error_from_bm_error(error);
    bm_ftp_send_end(coordinator.peer_node_id, coordinator.transfer_id,
                    error == BmOK, ftp_error, coordinator.bytes_received,
                    coordinator.crc16);
    bm_ftp_reset();
    return error;
  }

  uint32_t remaining = coordinator.total_size - coordinator.bytes_received;
  uint16_t requested_length = remaining < coordinator.chunk_size
                                  ? (uint16_t)remaining
                                  : coordinator.chunk_size;
  return bm_ftp_request_chunk(coordinator.peer_node_id, coordinator.transfer_id,
                              coordinator.bytes_received, requested_length);
}

static BmErr bm_ftp_handle_end(const BmFtpEvent *event) {
  const BmFtpEnd *end = (const BmFtpEnd *)event->payload;
  if (coordinator.state != BmFtpCoordinatorServing ||
      end->addresses.src_node_id != coordinator.peer_node_id ||
      end->transfer_id != coordinator.transfer_id) {
    return BmEBADMSG;
  }
  bm_ftp_reset();
  return end->success ? BmOK : BmECANCELED;
}

static BmErr bm_ftp_handle_abort(const BmFtpEvent *event) {
  const BmFtpAbort *abort = (const BmFtpAbort *)event->payload;
  if (coordinator.state == BmFtpCoordinatorIdle ||
      abort->addresses.src_node_id != coordinator.peer_node_id ||
      abort->transfer_id != coordinator.transfer_id) {
    return BmEBADMSG;
  }
  if (coordinator.state == BmFtpCoordinatorReceiving) {
    bm_ftp_sink_abort(&coordinator.sink);
  }
  bm_ftp_reset();
  return BmECANCELED;
}

BmErr bm_ftp_coordinator_process_event(const BmFtpEvent *event, void *context) {
  (void)context;
  switch (event->type) {
  case BmFtpEventStart:
    return bm_ftp_handle_start(event);
  case BmFtpEventFetch:
    return bm_ftp_handle_fetch(event);
  case BmFtpEventAck:
    return bm_ftp_handle_ack(event);
  case BmFtpEventChunkRequest:
    return bm_ftp_handle_chunk_request(event);
  case BmFtpEventChunk:
    return bm_ftp_handle_chunk(event);
  case BmFtpEventEnd:
    return bm_ftp_handle_end(event);
  case BmFtpEventAbort:
    return bm_ftp_handle_abort(event);
  default:
    return BmOK;
  }
}

BmErr bm_ftp_coordinator_init(void) {
  memset(&coordinator, 0, sizeof(coordinator));
  bm_ftp_set_event_handler(bm_ftp_coordinator_process_event, NULL);
  return BmOK;
}

BmErr bm_ftp_start_fetch(uint64_t source_node_id, uint32_t transfer_id,
                         BmFtpEndpointKind source_kind,
                         const uint8_t *source_spec, uint16_t source_spec_len,
                         BmFtpEndpointKind sink_kind, const uint8_t *sink_spec,
                         uint16_t sink_spec_len) {
  if (coordinator.state != BmFtpCoordinatorIdle ||
      (source_spec_len && !source_spec) || (sink_spec_len && !sink_spec) ||
      sink_spec_len > bm_ftp_max_endpoint_spec_len) {
    return BmEINVAL;
  }

  coordinator.state = BmFtpCoordinatorWaitingAck;
  coordinator.peer_node_id = source_node_id;
  coordinator.transfer_id = transfer_id;
  coordinator.pending_sink_kind = sink_kind;
  coordinator.pending_sink_spec_len = sink_spec_len;
  if (sink_spec_len) {
    memcpy(coordinator.pending_sink_spec, sink_spec, sink_spec_len);
  }

  BmErr error = bm_ftp_send_fetch(source_node_id, transfer_id, source_kind,
                                  source_spec, source_spec_len);
  if (error != BmOK) {
    bm_ftp_reset();
  }
  return error;
}
