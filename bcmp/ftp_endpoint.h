#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "ftp_message_structs.h"
#include "util.h"

typedef BmErr (*BmFtpReadAtFn)(void *context, uint32_t offset, uint8_t *buffer,
                               uint16_t length);
typedef BmErr (*BmFtpWriteAtFn)(void *context, uint32_t offset,
                                const uint8_t *data, uint16_t length);
typedef BmErr (*BmFtpCloseFn)(void *context);
typedef BmErr (*BmFtpFinalizeFn)(void *context, uint32_t total_size,
                                 uint16_t crc16);

typedef struct {
  void *context;
  BmFtpReadAtFn read_at;
  BmFtpCloseFn close;
} BmFtpSource;

typedef struct {
  void *context;
  BmFtpWriteAtFn write_at;
  BmFtpFinalizeFn finalize;
  BmFtpCloseFn abort;
  BmFtpCloseFn close;
} BmFtpSink;

typedef BmErr (*BmFtpOpenSourceFn)(const uint8_t *spec, uint16_t spec_len,
                                   BmFtpSource *source);
typedef BmErr (*BmFtpOpenSinkFn)(const uint8_t *spec, uint16_t spec_len,
                                 uint32_t total_size, BmFtpSink *sink);

typedef struct {
  BmFtpEndpointKind kind;
  BmFtpOpenSourceFn open_source;
  BmFtpOpenSinkFn open_sink;
} BmFtpEndpointOps;

BmErr bm_ftp_endpoint_register(const BmFtpEndpointOps *ops);
const BmFtpEndpointOps *bm_ftp_endpoint_lookup(BmFtpEndpointKind kind);

BmErr bm_ftp_source_open(BmFtpEndpointKind kind, const uint8_t *spec,
                         uint16_t spec_len, BmFtpSource *source);
BmErr bm_ftp_source_read_at(BmFtpSource *source, uint32_t offset,
                            uint8_t *buffer, uint16_t length);
BmErr bm_ftp_source_close(BmFtpSource *source);
BmErr bm_ftp_sink_open(BmFtpEndpointKind kind, const uint8_t *spec,
                       uint16_t spec_len, uint32_t total_size, BmFtpSink *sink);
BmErr bm_ftp_sink_write_at(BmFtpSink *sink, uint32_t offset, const uint8_t *data,
                           uint16_t length);
BmErr bm_ftp_sink_finalize(BmFtpSink *sink, uint32_t total_size, uint16_t crc16);
BmErr bm_ftp_sink_abort(BmFtpSink *sink);
BmErr bm_ftp_sink_close(BmFtpSink *sink);

#ifdef __cplusplus
}
#endif