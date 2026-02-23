#include <gtest/gtest.h>
#include <helpers.hpp>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "fff.h"

DEFINE_FFF_GLOBALS;

extern "C" {
#include "mock_bm_os.h"
#include "mock_device.h"
#include "mock_l2.h"
#include "mock_packet.h"
#include "util.h"

/* bm_linux.c helpers exposed via BM_LINUX_STATIC when ENABLE_TESTING is set */
void nodeid_to_ip(BmIpAddr *out, uint32_t prefix, uint64_t id);
void format_ipv6(char *out, const BmIpAddr *addr);
uint16_t ipv6_pseudo_checksum(const BmIpAddr *src, const BmIpAddr *dst,
                              uint8_t next_header, uint32_t length,
                              const void *data);
void mac_from_nodeid(uint8_t *mac, uint64_t id);
void multicast_mac_from_ipv6(uint8_t *mac, const BmIpAddr *dst);
bool is_multicast(const BmIpAddr *addr);

/* Public API under test */
#include "bm_ip.h"

/* bcmp_get_queue is called by bm_l2_submit but not in mock_bcmp.h */
DECLARE_FAKE_VALUE_FUNC(void *, bcmp_get_queue);
} /* extern "C" */

DEFINE_FAKE_VALUE_FUNC(void *, bcmp_get_queue);

// ============================================================================
// Helper function tests
// ============================================================================

class BmLinuxHelpers : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(BmLinuxHelpers, nodeid_to_ip_roundtrip) {
  uint64_t id = 0xDEADBEEF12345678ULL;
  BmIpAddr addr;
  nodeid_to_ip(&addr, 0xFE800000, id);

  /* Prefix bytes */
  EXPECT_EQ(addr.addr[0], 0xFE);
  EXPECT_EQ(addr.addr[1], 0x80);
  EXPECT_EQ(addr.addr[2], 0x00);
  EXPECT_EQ(addr.addr[3], 0x00);
  /* Bytes 4-7 are zero */
  for (int i = 4; i < 8; i++) {
    EXPECT_EQ(addr.addr[i], 0x00);
  }
  /* Roundtrip: ip_to_nodeid should recover the original id */
  EXPECT_EQ(ip_to_nodeid(&addr), id);
}

TEST_F(BmLinuxHelpers, nodeid_to_ip_fd00_prefix) {
  uint64_t id = 0x0000000000000001ULL;
  BmIpAddr addr;
  nodeid_to_ip(&addr, 0xFD000000, id);
  EXPECT_EQ(addr.addr[0], 0xFD);
  EXPECT_EQ(addr.addr[1], 0x00);
  EXPECT_EQ(ip_to_nodeid(&addr), id);
}

TEST_F(BmLinuxHelpers, format_ipv6_link_local) {
  uint64_t id = 0x0000000100000002ULL;
  BmIpAddr addr;
  nodeid_to_ip(&addr, 0xFE800000, id);
  char buf[40] = {};
  format_ipv6(buf, &addr);
  EXPECT_STREQ(buf, "fe80::1:2");
}

TEST_F(BmLinuxHelpers, format_ipv6_all_zeros) {
  BmIpAddr addr;
  memset(&addr, 0, sizeof(addr));
  char buf[40] = {};
  format_ipv6(buf, &addr);
  EXPECT_STREQ(buf, "::");
}

TEST_F(BmLinuxHelpers, mac_from_nodeid_locally_administered) {
  uint64_t id = 0xAABBCCDDEEFF0011ULL;
  uint8_t mac[6];
  mac_from_nodeid(mac, id);
  /* Bit 1 of byte 0 must be set (locally administered) */
  EXPECT_TRUE(mac[0] & 0x02);
}

TEST_F(BmLinuxHelpers, multicast_mac_from_ipv6_prefix) {
  BmIpAddr addr;
  memset(&addr, 0, sizeof(addr));
  addr.addr[0] = 0xFF;
  addr.addr[1] = 0x03;
  addr.addr[12] = 0xAA;
  addr.addr[13] = 0xBB;
  addr.addr[14] = 0xCC;
  addr.addr[15] = 0xDD;
  uint8_t mac[6];
  multicast_mac_from_ipv6(mac, &addr);
  EXPECT_EQ(mac[0], 0x33);
  EXPECT_EQ(mac[1], 0x33);
  EXPECT_EQ(mac[2], 0xAA);
  EXPECT_EQ(mac[3], 0xBB);
  EXPECT_EQ(mac[4], 0xCC);
  EXPECT_EQ(mac[5], 0xDD);
}

TEST_F(BmLinuxHelpers, is_multicast_true) {
  BmIpAddr addr;
  memset(&addr, 0, sizeof(addr));
  addr.addr[0] = 0xFF;
  EXPECT_TRUE(is_multicast(&addr));
}

TEST_F(BmLinuxHelpers, is_multicast_false) {
  BmIpAddr addr;
  memset(&addr, 0, sizeof(addr));
  addr.addr[0] = 0xFE;
  EXPECT_FALSE(is_multicast(&addr));
}

TEST_F(BmLinuxHelpers, ipv6_pseudo_checksum_nonzero) {
  BmIpAddr src, dst;
  memset(&src, 0, sizeof(src));
  memset(&dst, 0, sizeof(dst));
  src.addr[0] = 0xFE;
  src.addr[1] = 0x80;
  dst.addr[0] = 0xFF;
  dst.addr[1] = 0x03;
  dst.addr[15] = 0x01;
  uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
  uint16_t cksum = ipv6_pseudo_checksum(&src, &dst, 0xBC, sizeof(data), data);
  /* Just verify it's non-zero and deterministic */
  EXPECT_NE(cksum, 0);
  uint16_t cksum2 = ipv6_pseudo_checksum(&src, &dst, 0xBC, sizeof(data), data);
  EXPECT_EQ(cksum, cksum2);
}

// ============================================================================
// Buffer lifecycle tests
// ============================================================================

class BmLinuxBuf : public ::testing::Test {
protected:
  void SetUp() override { RESET_FAKE(bm_l2_link_output); }
  void TearDown() override {}
};

TEST_F(BmLinuxBuf, l2_new_and_payload) {
  void *buf = bm_l2_new(64);
  ASSERT_NE(buf, nullptr);
  void *payload = bm_l2_get_payload(buf);
  ASSERT_NE(payload, nullptr);
  /* Write and read back */
  memset(payload, 0xAB, 64);
  EXPECT_EQ(((uint8_t *)payload)[0], 0xAB);
  EXPECT_EQ(((uint8_t *)payload)[63], 0xAB);
  bm_l2_free(buf);
}

TEST_F(BmLinuxBuf, l2_ref_counting) {
  void *buf = bm_l2_new(32);
  ASSERT_NE(buf, nullptr);
  /* ref starts at 1; tx_prep increments to 2 */
  bm_l2_tx_prep(buf, 0);
  /* First free: ref goes to 1, not freed */
  bm_l2_free(buf);
  /* Should still be accessible (ref=1) */
  void *p = bm_l2_get_payload(buf);
  EXPECT_NE(p, nullptr);
  /* Second free: ref goes to 0, actually freed */
  bm_l2_free(buf);
  EXPECT_EQ(p, nullptr);
}

TEST_F(BmLinuxBuf, l2_free_null_safe) {
  /* Should not crash */
  bm_l2_free(NULL);
}

TEST_F(BmLinuxBuf, l2_get_payload_null) {
  EXPECT_EQ(bm_l2_get_payload(NULL), nullptr);
}

TEST_F(BmLinuxBuf, udp_new_delegates_to_l2) {
  void *buf = bm_udp_new(128);
  ASSERT_NE(buf, nullptr);
  void *payload = bm_udp_get_payload(buf);
  ASSERT_NE(payload, nullptr);
  bm_udp_cleanup(buf);
}

TEST_F(BmLinuxBuf, udp_reference_update) {
  void *buf = bm_udp_new(16);
  ASSERT_NE(buf, nullptr);
  EXPECT_EQ(bm_udp_reference_update(buf), BmOK);
  /* ref is now 2 — first cleanup decrements to 1 */
  bm_udp_cleanup(buf);
  /* Still alive */
  EXPECT_NE(bm_udp_get_payload(buf), nullptr);
  bm_udp_cleanup(buf);
}

TEST_F(BmLinuxBuf, udp_reference_update_null) {
  EXPECT_NE(bm_udp_reference_update(NULL), BmOK);
}

TEST_F(BmLinuxBuf, buf_shrink) {
  void *buf = bm_l2_new(256);
  ASSERT_NE(buf, nullptr);
  bm_ip_buf_shrink(buf, 64);
  /* No crash; internal len updated (not directly observable but exercises path) */
  bm_l2_free(buf);
}

TEST_F(BmLinuxBuf, set_netif) {
  EXPECT_EQ(bm_l2_set_netif(true), BmOK);
  EXPECT_EQ(bm_l2_set_netif(false), BmOK);
}
