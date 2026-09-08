#include <gtest/gtest.h>

#include <vector>

#include "fff.h"

extern "C" {
#include "ftp.h"
#include "mock_bcmp.h"
#include "mock_device.h"
}

DEFINE_FFF_GLOBALS;

namespace {

BcmpMessageType transmitted_type;
std::vector<uint8_t> transmitted_payload;

BmErr capture_transmit(const BmIpAddr *destination, BcmpMessageType type, uint8_t *payload,
                       uint16_t payload_len, uint32_t sequence_number, BcmpReplyCb callback) {
  (void)destination;
  (void)sequence_number;
  (void)callback;
  transmitted_type = type;
  transmitted_payload.assign(payload, payload + payload_len);
  return BmOK;
}

class FtpCore : public ::testing::Test {
protected:
  void SetUp() override {
    RESET_FAKE(bcmp_tx);
    RESET_FAKE(node_id);
    transmitted_payload.clear();
    bcmp_tx_fake.custom_fake = capture_transmit;
    node_id_fake.return_val = 0x0123456789abcdef;
  }
};

} // namespace

TEST_F(FtpCore, send_start_encodes_sink_descriptor) {
  const uint8_t sink_spec[] = {'n', 'v', 'm', '0'};

  ASSERT_EQ(bm_ftp_send_start(0xfedcba9876543210, 7, 1024, 256, 0x1234,
                              BmFtpEndpointNvm, sink_spec, sizeof(sink_spec)),
            BmOK);
  ASSERT_EQ(transmitted_type, BcmpFTPStartMessage);
  ASSERT_EQ(transmitted_payload.size(), sizeof(BmFtpStart) + sizeof(sink_spec));

  const auto *start = reinterpret_cast<const BmFtpStart *>(transmitted_payload.data());
  EXPECT_EQ(start->addresses.src_node_id, node_id_fake.return_val);
  EXPECT_EQ(start->addresses.dst_node_id, 0xfedcba9876543210);
  EXPECT_EQ(start->transfer_id, 7U);
  EXPECT_EQ(start->total_size, 1024U);
  EXPECT_EQ(start->requested_chunk_size, 256U);
  EXPECT_EQ(start->crc16, 0x1234U);
  EXPECT_EQ(start->sink_kind, BmFtpEndpointNvm);
  EXPECT_EQ(start->sink_spec_len, sizeof(sink_spec));
  EXPECT_EQ(memcmp(start->sink_spec, sink_spec, sizeof(sink_spec)), 0);
}

TEST_F(FtpCore, send_fetch_encodes_source_descriptor) {
  const uint8_t source_spec[] = {'/', 'l', 'o', 'g'};

  ASSERT_EQ(bm_ftp_send_fetch(0xfedcba9876543210, 9, BmFtpEndpointFlash, source_spec,
                              sizeof(source_spec)),
            BmOK);
  ASSERT_EQ(transmitted_type, BcmpFTPFetchMessage);
  ASSERT_EQ(transmitted_payload.size(), sizeof(BmFtpFetch) + sizeof(source_spec));

  const auto *fetch = reinterpret_cast<const BmFtpFetch *>(transmitted_payload.data());
  EXPECT_EQ(fetch->addresses.src_node_id, node_id_fake.return_val);
  EXPECT_EQ(fetch->addresses.dst_node_id, 0xfedcba9876543210);
  EXPECT_EQ(fetch->transfer_id, 9U);
  EXPECT_EQ(fetch->source_kind, BmFtpEndpointFlash);
  EXPECT_EQ(fetch->source_spec_len, sizeof(source_spec));
  EXPECT_EQ(memcmp(fetch->source_spec, source_spec, sizeof(source_spec)), 0);
}

TEST_F(FtpCore, send_helpers_reject_missing_variable_payload) {
  EXPECT_EQ(bm_ftp_send_start(1, 1, 1, 1, 1, BmFtpEndpointNvm, nullptr, 1), BmEINVAL);
  EXPECT_EQ(bm_ftp_send_fetch(1, 1, BmFtpEndpointFlash, nullptr, 1), BmEINVAL);
  EXPECT_EQ(bm_ftp_send_chunk(1, 1, 0, nullptr, 1), BmEINVAL);
}