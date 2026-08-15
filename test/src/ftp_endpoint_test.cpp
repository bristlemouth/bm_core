#include <gtest/gtest.h>

extern "C" {
#include "ftp_endpoint.h"
}

namespace {

uint8_t source_context;
uint8_t sink_context;
uint32_t source_read_offset;
uint32_t sink_write_offset;
uint16_t sink_write_length;
uint32_t finalized_size;
uint16_t finalized_crc16;

BmErr source_read_at(void *context, uint32_t offset, uint8_t *buffer, uint16_t length) {
  if (context != &source_context) {
    return BmEINVAL;
  }
  source_read_offset = offset;
  for (uint16_t index = 0; index < length; index++) {
    buffer[index] = static_cast<uint8_t>(index);
  }
  return BmOK;
}

BmErr source_close(void *context) { return context == &source_context ? BmOK : BmEINVAL; }

BmErr sink_write_at(void *context, uint32_t offset, const uint8_t *data, uint16_t length) {
  if (context != &sink_context || (length && !data)) {
    return BmEINVAL;
  }
  sink_write_offset = offset;
  sink_write_length = length;
  return BmOK;
}

BmErr sink_finalize(void *context, uint32_t total_size, uint16_t crc16) {
  if (context != &sink_context) {
    return BmEINVAL;
  }
  finalized_size = total_size;
  finalized_crc16 = crc16;
  return BmOK;
}

BmErr sink_close(void *context) { return context == &sink_context ? BmOK : BmEINVAL; }

BmErr open_source(const uint8_t *spec, uint16_t spec_len, BmFtpSource *source) {
  if (!spec || spec_len != 3) {
    return BmEINVAL;
  }
  source->context = &source_context;
  source->read_at = source_read_at;
  source->close = source_close;
  return BmOK;
}

BmErr open_sink(const uint8_t *spec, uint16_t spec_len, uint32_t total_size, BmFtpSink *sink) {
  if (!spec || spec_len != 3 || total_size != 16) {
    return BmEINVAL;
  }
  sink->context = &sink_context;
  sink->write_at = sink_write_at;
  sink->finalize = sink_finalize;
  sink->abort = sink_close;
  sink->close = sink_close;
  return BmOK;
}

} // namespace

TEST(FtpEndpoint, registered_backend_opens_source_and_sink) {
  const BmFtpEndpointOps ops = {
      .kind = BmFtpEndpointNvm,
      .open_source = open_source,
      .open_sink = open_sink,
  };
  const uint8_t spec[] = {'n', 'v', 'm'};
  uint8_t buffer[4] = {};
  BmFtpSource source = {};
  BmFtpSink sink = {};

  ASSERT_EQ(bm_ftp_endpoint_register(&ops), BmOK);
  EXPECT_EQ(bm_ftp_endpoint_register(&ops), BmEALREADY);

  ASSERT_EQ(bm_ftp_source_open(BmFtpEndpointNvm, spec, sizeof(spec), &source), BmOK);
  ASSERT_EQ(bm_ftp_source_read_at(&source, 4, buffer, sizeof(buffer)), BmOK);
  EXPECT_EQ(source_read_offset, 4U);
  EXPECT_EQ(buffer[3], 3U);
  EXPECT_EQ(bm_ftp_source_close(&source), BmOK);

  ASSERT_EQ(bm_ftp_sink_open(BmFtpEndpointNvm, spec, sizeof(spec), 16, &sink), BmOK);
  ASSERT_EQ(bm_ftp_sink_write_at(&sink, 8, buffer, sizeof(buffer)), BmOK);
  EXPECT_EQ(sink_write_offset, 8U);
  EXPECT_EQ(sink_write_length, sizeof(buffer));
  ASSERT_EQ(bm_ftp_sink_finalize(&sink, 16, 0x1234), BmOK);
  EXPECT_EQ(finalized_size, 16U);
  EXPECT_EQ(finalized_crc16, 0x1234U);
  EXPECT_EQ(bm_ftp_sink_close(&sink), BmOK);
}

TEST(FtpEndpoint, unregistered_backend_is_not_available) {
  BmFtpSource source = {};
  BmFtpSink sink = {};

  EXPECT_EQ(bm_ftp_source_open(BmFtpEndpointFlash, nullptr, 0, &source), BmENODEV);
  EXPECT_EQ(bm_ftp_sink_open(BmFtpEndpointFlash, nullptr, 0, 0, &sink), BmENODEV);
}