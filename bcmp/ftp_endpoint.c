#include "ftp_endpoint.h"

#include <string.h>

#define bm_ftp_endpoint_count (3U)

static const BmFtpEndpointOps *endpoint_ops[bm_ftp_endpoint_count];

BmErr bm_ftp_endpoint_register(const BmFtpEndpointOps *ops) {
  if (!ops || ops->kind >= bm_ftp_endpoint_count ||
      (!ops->open_source && !ops->open_sink)) {
    return BmEINVAL;
  }
  if (endpoint_ops[ops->kind]) {
    return BmEALREADY;
  }
  endpoint_ops[ops->kind] = ops;
  return BmOK;
}

const BmFtpEndpointOps *bm_ftp_endpoint_lookup(BmFtpEndpointKind kind) {
  return kind < bm_ftp_endpoint_count ? endpoint_ops[kind] : NULL;
}

BmErr bm_ftp_source_open(BmFtpEndpointKind kind, const uint8_t *spec,
                         uint16_t spec_len, BmFtpSource *source) {
  const BmFtpEndpointOps *ops = bm_ftp_endpoint_lookup(kind);
  if (!source || (spec_len && !spec)) {
    return BmEINVAL;
  }
  if (!ops || !ops->open_source) {
    return BmENODEV;
  }
  memset(source, 0, sizeof(*source));
  return ops->open_source(spec, spec_len, source);
}

BmErr bm_ftp_source_read_at(BmFtpSource *source, uint32_t offset,
                            uint8_t *buffer, uint16_t length) {
  if (!source || !source->read_at || (length && !buffer)) {
    return BmEINVAL;
  }
  return source->read_at(source->context, offset, buffer, length);
}

BmErr bm_ftp_source_close(BmFtpSource *source) {
  if (!source || !source->close) {
    return BmEINVAL;
  }
  BmErr err = source->close(source->context);
  memset(source, 0, sizeof(*source));
  return err;
}

BmErr bm_ftp_sink_open(BmFtpEndpointKind kind, const uint8_t *spec,
                       uint16_t spec_len, uint32_t total_size,
                       BmFtpSink *sink) {
  const BmFtpEndpointOps *ops = bm_ftp_endpoint_lookup(kind);
  if (!sink || (spec_len && !spec)) {
    return BmEINVAL;
  }
  if (!ops || !ops->open_sink) {
    return BmENODEV;
  }
  memset(sink, 0, sizeof(*sink));
  return ops->open_sink(spec, spec_len, total_size, sink);
}

BmErr bm_ftp_sink_write_at(BmFtpSink *sink, uint32_t offset, const uint8_t *data,
                           uint16_t length) {
  if (!sink || !sink->write_at || (length && !data)) {
    return BmEINVAL;
  }
  return sink->write_at(sink->context, offset, data, length);
}

BmErr bm_ftp_sink_finalize(BmFtpSink *sink, uint32_t total_size, uint16_t crc16) {
  if (!sink || !sink->finalize) {
    return BmEINVAL;
  }
  return sink->finalize(sink->context, total_size, crc16);
}

BmErr bm_ftp_sink_abort(BmFtpSink *sink) {
  if (!sink || !sink->abort) {
    return BmEINVAL;
  }
  return sink->abort(sink->context);
}

BmErr bm_ftp_sink_close(BmFtpSink *sink) {
  if (!sink || !sink->close) {
    return BmEINVAL;
  }
  BmErr err = sink->close(sink->context);
  memset(sink, 0, sizeof(*sink));
  return err;
}
