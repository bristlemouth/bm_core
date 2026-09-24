#include <gtest/gtest.h>

#include <vector>

#include "fff.h"

extern "C" {
#include "ftp.h"
#include "ftp_endpoint.h"
#include "mock_bcmp.h"
#include "mock_device.h"
}

DEFINE_FFF_GLOBALS;

namespace {

BcmpMessageType transmitted_type;
std::vector<uint8_t> transmitted_payload;
uint32_t opened_sink_size;

BmErr capture_transmit(const BmIpAddr *destination, BcmpMessageType type, uint8_t *payload,
                       uint16_t payload_len, uint32_t sequence_number, BcmpReplyCb callback) {
  (void)destination;
  (void)sequence_number;
  (void)callback;
  transmitted_type = type;
  transmitted_payload.assign(payload, payload + payload_len);
  return BmOK;
}

BmErr sink_write(void *context, uint32_t offset, const uint8_t *data, uint16_t length) {
  (void)context;
  (void)offset;
  (void)data;
  (void)length;
  return BmOK;
}

BmErr sink_finalize(void *context, uint32_t total_size, uint16_t crc16) {
  (void)context;
  (void)total_size;
  (void)crc16;
  return BmOK;
}

BmErr sink_close(void *context) {
  (void)context;
  return BmOK;
}

BmErr open_sink(const uint8_t *spec, uint16_t spec_len, uint32_t total_size, BmFtpSink *sink) {
  if (!spec || spec_len != 3) {
    return BmEINVAL;
  }
  opened_sink_size = total_size;
  sink->context = nullptr;
  sink->write_at = sink_write;
  sink->finalize = sink_finalize;
  sink->abort = sink_close;
  sink->close = sink_close;
  return BmOK;
}

const BmFtpEndpointOps sink_ops = {
    .kind = BmFtpEndpointNvm,
    .open_source = nullptr,
    .open_sink = open_sink,
};

class FtpCoordinator : public ::testing::Test {
protected:
  void SetUp() override {
    RESET_FAKE(bcmp_tx);
    RESET_FAKE(node_id);
    bcmp_tx_fake.custom_fake = capture_transmit;
    node_id_fake.return_val = 0x0123456789abcdef;
    transmitted_payload.clear();
    opened_sink_size = 0;

    ASSERT_EQ(bm_ftp_endpoint_register(&sink_ops), BmOK);
    ASSERT_EQ(bm_ftp_coordinator_init(), BmOK);
  }
};

} // namespace

TEST_F(FtpCoordinator, fetch_ack_opens_sink_and_requests_first_chunk) {
  const uint8_t source_spec[] = {'l', 'o', 'g'};
  const uint8_t sink_spec[] = {'n', 'v', 'm'};

  ASSERT_EQ(bm_ftp_start_fetch(0xfedcba9876543210, 42, BmFtpEndpointFlash, source_spec,
                              sizeof(source_spec), BmFtpEndpointNvm, sink_spec,
                              sizeof(sink_spec)),
            BmOK);
  ASSERT_EQ(transmitted_type, BcmpFTPFetchMessage);

  BmFtpAck ack = {
      .addresses = {.src_node_id = 0xfedcba9876543210,
                    .dst_node_id = node_id_fake.return_val},
      .transfer_id = 42,
      .total_size = 100,
      .crc16 = 0x1234,
      .chunk_size = 32,
      .success = true,
      .err_code = BmFtpErrNone,
  };
  BmFtpEvent event = {
      .type = BmFtpEventAck,
      .payload = reinterpret_cast<uint8_t *>(&ack),
      .payload_len = sizeof(ack),
  };

  ASSERT_EQ(bm_ftp_coordinator_process_event(&event, nullptr), BmOK);
  EXPECT_EQ(opened_sink_size, 100U);
  ASSERT_EQ(transmitted_type, BcmpFTPChunkReqMessage);

  const auto *request = reinterpret_cast<const BmFtpChunkRequest *>(transmitted_payload.data());
  EXPECT_EQ(request->addresses.dst_node_id, 0xfedcba9876543210);
  EXPECT_EQ(request->transfer_id, 42U);
  EXPECT_EQ(request->offset, 0U);
  EXPECT_EQ(request->length, 32U);
}
