#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


// I'm not sure if I want these, or if I want
// the nodes to define them arbitrarily themselves.
typedef enum {
  BmFtpEndpointFlash = 0,
  BmFtpEndpointNvm = 1,
  BmFtpEndpointBmSerial = 2,
} BmFtpEndpointKind;

typedef enum {
  BmFtpErrNone = 0,
  BmFtpErrInvalidEndpoint = 1,
  BmFtpErrInvalidSpec = 2,
  BmFtpErrBusy = 3,
  BmFtpErrTooLarge = 4,
  BmFtpErrBadChunk = 5,
  BmFtpErrWriteFailed = 6,
  BmFtpErrCrcMismatch = 7,
  BmFtpErrTimeout = 8,
  BmFtpErrAborted = 9,
  BmFtpErrUnsupported = 10,
} BmFtpErr;

typedef struct __attribute__((__packed__)) {
  uint64_t src_node_id;
  uint64_t dst_node_id;
} BmFtpAddress;

/* START: source-initiated transfer request. */
typedef struct __attribute__((__packed__)) {
  BmFtpAddress addresses;
  uint32_t transfer_id;
  uint32_t total_size;
  uint16_t requested_chunk_size;
  uint16_t crc16;
  uint8_t sink_kind;
  uint8_t flags;
  uint16_t sink_spec_len;
  uint8_t sink_spec[0];
} BmFtpStart;

/* FETCH: sink-initiated transfer request. */
typedef struct __attribute__((__packed__)) {
  BmFtpAddress addresses;
  uint32_t transfer_id;
  uint8_t source_kind;
  uint8_t flags;
  uint16_t source_spec_len;
  uint8_t source_spec[0];
} BmFtpFetch;

/* ACK: acceptance or rejection of START or FETCH. */
typedef struct __attribute__((__packed__)) {
  BmFtpAddress addresses;
  uint32_t transfer_id;
  uint32_t total_size;
  uint16_t crc16;
  uint16_t chunk_size;
  uint8_t success;
  uint8_t err_code;
} BmFtpAck;

typedef struct __attribute__((__packed__)) {
  BmFtpAddress addresses;
  uint32_t transfer_id;
  uint32_t offset;
  uint16_t length;
  uint16_t reserved;
} BmFtpChunkRequest;

typedef struct __attribute__((__packed__)) {
  BmFtpAddress addresses;
  uint32_t transfer_id;
  uint32_t offset;
  uint16_t payload_length;
  uint16_t reserved;
  uint8_t payload[0];
} BmFtpChunk;

typedef struct __attribute__((__packed__)) {
  BmFtpAddress addresses;
  uint32_t transfer_id;
  uint8_t success;
  uint8_t err_code;
  uint16_t reserved;
  uint32_t bytes_received;
  uint16_t running_crc16;
  uint16_t reserved2;
} BmFtpEnd;

typedef struct __attribute__((__packed__)) {
  BmFtpAddress addresses;
  uint32_t transfer_id;
  uint8_t err_code;
  uint8_t reserved[3];
} BmFtpAbort;

#ifdef __cplusplus
}
#endif