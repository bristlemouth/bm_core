#include "fff.h"
#include "gtest/gtest.h"
#include <cstring>
#include <vector>

DEFINE_FFF_GLOBALS

extern "C" {
#include "mock_bm_os.h"
#include "pcap.h"
}

// Captured bytes from the write callback.
static std::vector<uint8_t> s_captured;

static void capture_cb(const uint8_t *data, size_t len, void *ctx) {
  (void)ctx;
  s_captured.insert(s_captured.end(), data, data + len);
}

class Pcap : public ::testing::Test {
protected:
  void SetUp() override {
    s_captured.clear();
    RESET_FAKE(bm_get_tick_count);
    RESET_FAKE(bm_ticks_to_ms);
  }
};

// Verify the 24-byte global header matches what Wireshark expects.
TEST_F(Pcap, global_header) {
  pcap_init(capture_cb, nullptr);

  ASSERT_EQ(s_captured.size(), 24u);

  // Read fields assuming little-endian host (pcap native byte order).
  uint32_t magic;
  uint16_t ver_major, ver_minor;
  uint32_t snaplen, network;
  memcpy(&magic, &s_captured[0], 4);
  memcpy(&ver_major, &s_captured[4], 2);
  memcpy(&ver_minor, &s_captured[6], 2);
  memcpy(&snaplen, &s_captured[16], 4);
  memcpy(&network, &s_captured[20], 4);

  EXPECT_EQ(magic, 0xA1B2C3D4u);
  EXPECT_EQ(ver_major, 2);
  EXPECT_EQ(ver_minor, 4);
  EXPECT_EQ(snaplen, 65535u);
  EXPECT_EQ(network, 1u); // LINKTYPE_ETHERNET
}

// Verify a packet record has correct header fields and payload.
TEST_F(Pcap, packet_record) {
  // Configure tick stubs: 5500 ms elapsed.
  bm_get_tick_count_fake.return_val = 42;
  bm_ticks_to_ms_fake.return_val = 5500;

  pcap_init(capture_cb, nullptr);
  s_captured.clear(); // discard global header

  const uint8_t frame[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02};
  pcap_write_packet(frame, sizeof(frame));

  // 16-byte record header + 6-byte payload
  ASSERT_EQ(s_captured.size(), 16u + sizeof(frame));

  uint32_t ts_sec, ts_usec, incl_len, orig_len;
  memcpy(&ts_sec, &s_captured[0], 4);
  memcpy(&ts_usec, &s_captured[4], 4);
  memcpy(&incl_len, &s_captured[8], 4);
  memcpy(&orig_len, &s_captured[12], 4);

  EXPECT_EQ(ts_sec, 5u);       // 5500 / 1000
  EXPECT_EQ(ts_usec, 500000u); // (5500 % 1000) * 1000
  EXPECT_EQ(incl_len, sizeof(frame));
  EXPECT_EQ(orig_len, sizeof(frame));

  EXPECT_EQ(memcmp(&s_captured[16], frame, sizeof(frame)), 0);
}

// Null/empty inputs must not crash or produce output.
TEST_F(Pcap, write_packet_null_and_empty) {
  pcap_init(capture_cb, nullptr);
  s_captured.clear();

  pcap_write_packet(nullptr, 10);
  EXPECT_TRUE(s_captured.empty());

  const uint8_t frame[] = {0x01};
  pcap_write_packet(frame, 0);
  EXPECT_TRUE(s_captured.empty());
}

// Writing before init must not crash.
TEST_F(Pcap, write_before_init) {
  const uint8_t frame[] = {0x01};
  // No pcap_init called — s_write_cb is NULL from a prior test's teardown
  // or from process start. Just verify no crash.
  pcap_write_packet(frame, sizeof(frame));
}
