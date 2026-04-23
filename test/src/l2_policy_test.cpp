#include <gtest/gtest.h>
#include <helpers.hpp>
#include <stdint.h>
#include <string.h>

#include "fff.h"
DEFINE_FFF_GLOBALS;

extern "C" {
#include "l2_policy.h"
#include "util.h"
}

// --- Local frame layout helpers (mirror the offsets used by l2.c) ---
// These offsets are derived from the same definitions present in l2.c
// (Ethernet header + IPv6 header; src starts at 22, dst starts at 38; ports byte = src+2).
static constexpr size_t ETH_TYPE_OFFSET = 12;
static constexpr size_t IPV6_HEADER_OFFSET = 14;
static constexpr size_t IPV6_SRC_OFFSET =
    IPV6_HEADER_OFFSET + 4 + 2 + 1 + 1;                          // 14 + 8 = 22
static constexpr size_t IPV6_DST_OFFSET = IPV6_SRC_OFFSET + 16;  // 38
static constexpr size_t PORTS_BYTE_OFFSET = IPV6_SRC_OFFSET + 2; // src + 2

static constexpr size_t MIN_FRAME_LEN = IPV6_DST_OFFSET + 16;

static void write_ethertype_ipv6(uint8_t *frame) {
  // EtherType IPv6 = 0x86DD
  frame[ETH_TYPE_OFFSET + 0] = 0x86;
  frame[ETH_TYPE_OFFSET + 1] = 0xDD;
}

static void write_ipv6_dst(uint8_t *frame, const uint8_t dst[16]) {
  memcpy(&frame[IPV6_DST_OFFSET], dst, 16);
}

static void write_ipv6_src(uint8_t *frame, const uint8_t src[16]) {
  memcpy(&frame[IPV6_SRC_OFFSET], src, 16);
}

static uint8_t ports_byte(const uint8_t *frame) {
  return frame[PORTS_BYTE_OFFSET];
}
static uint8_t ingress_nibble(const uint8_t *frame) {
  return (uint8_t)((ports_byte(frame) >> 4) & 0x0F);
}
static uint8_t egress_nibble(const uint8_t *frame) {
  return (uint8_t)(ports_byte(frame) & 0x0F);
}

// --- fff fake routing callback ---
FAKE_VALUE_FUNC4(bool, routing_cb, uint8_t, uint16_t *, BmIpAddr *,
                 const BmIpAddr *);

static bool routing_cb_impl_forward_only(uint8_t ingress_port,
                                         uint16_t *egress_mask, BmIpAddr *src,
                                         const BmIpAddr *dest) {
  (void)ingress_port;
  (void)src;
  (void)dest;

  // Deterministic: forward to port 2 only, do NOT submit upward.
  *egress_mask = (1U << (2 - 1));
  return false;
}

static bool routing_cb_impl_forward_and_submit(uint8_t ingress_port,
                                               uint16_t *egress_mask,
                                               BmIpAddr *src,
                                               const BmIpAddr *dest) {
  (void)ingress_port;
  (void)src;
  (void)dest;

  // Deterministic: forward to ports 1 and 2, DO submit upward.
  *egress_mask = (uint16_t)((1U << (1 - 1)) | (1U << (2 - 1)));
  return true;
}

class L2Policy : public ::testing::Test {
protected:
  void SetUp() override {
    RESET_FAKE(routing_cb);
    routing_cb_fake.custom_fake = routing_cb_impl_forward_only;
  }
  void TearDown() override { RESET_FAKE(routing_cb); }
};

TEST_F(L2Policy,
       GlobalMulticast_setsIngress_preservesEgress_returnsForwardMask) {
  uint8_t frame[MIN_FRAME_LEN] = {0};
  write_ethertype_ipv6(frame);

  uint8_t src[16] = {0};
  uint8_t dst_ff03_1[16] = {
      0xFF, 0x03, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01,
  };

  write_ipv6_src(frame, src);
  write_ipv6_dst(frame, dst_ff03_1);

  // Simulate "on-wire": ingress=0, egress=0xE
  frame[PORTS_BYTE_OFFSET] = 0x0E;

  const uint16_t ingress_mask = (1U << (3 - 1)); // port 3
  const uint16_t all_ports_mask = 0x000F;        // ports 1..4

  const BmL2PolicyRxResult r = bm_l2_policy_rx_apply(
      frame, sizeof(frame), ingress_mask, all_ports_mask, routing_cb);

  // Global multicast => forward to all except ingress
  EXPECT_EQ(r.egress_mask,
            (uint16_t)(all_ports_mask & (uint16_t)~ingress_mask));
  EXPECT_TRUE(r.should_submit);
  EXPECT_EQ(r.ingress_port_num, 3);

  // Ingress encoded for app-visible buffer
  EXPECT_EQ(ingress_nibble(frame), 3);

  // Egress preserved on this path (no routing_cb)
  EXPECT_EQ(egress_nibble(frame), 0x0E);

  // Callback not used on global multicast
  EXPECT_EQ(routing_cb_fake.call_count, 0);
}

TEST_F(
    L2Policy,
    LinkLocalNonNeighbor_invokesRoutingCb_clearsEgressNibble_respectsSubmitFlag) {
  uint8_t frame[MIN_FRAME_LEN] = {0};
  write_ethertype_ipv6(frame);

  // ff02::2 (link-local multicast, not neighbor multicast ff02::1)
  uint8_t dst_ff02_2[16] = {
      0xFF, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x02,
  };

  uint8_t src[16] = {0};
  write_ipv6_src(frame, src);
  write_ipv6_dst(frame, dst_ff02_2);

  // start with ingress junk + egress=0xE so we can see what changes
  frame[PORTS_BYTE_OFFSET] = 0xAE;

  const uint16_t ingress_mask = (1U << (4 - 1)); // port 4
  const uint16_t all_ports_mask = 0x000F;

  const BmL2PolicyRxResult r = bm_l2_policy_rx_apply(
      frame, sizeof(frame), ingress_mask, all_ports_mask, routing_cb);

  EXPECT_EQ(routing_cb_fake.call_count, 1);

  // From routing_cb_impl_forward_only
  EXPECT_EQ(r.egress_mask, (uint16_t)(1U << (2 - 1)));
  EXPECT_FALSE(r.should_submit);
  EXPECT_EQ(r.ingress_port_num, 4);

  // Ingress nibble overwritten with actual ingress port
  EXPECT_EQ(ingress_nibble(frame), 4);

  // Egress nibble cleared after callback
  EXPECT_EQ(egress_nibble(frame), 0);
}

TEST_F(L2Policy,
       LinkLocalNeighbor_ff02_1_bypassesRoutingCb_doesNotForward_submits) {
  uint8_t frame[MIN_FRAME_LEN] = {0};
  write_ethertype_ipv6(frame);

  // ff02::1 (neighbor multicast / all-nodes)
  uint8_t dst_ff02_1[16] = {
      0xFF, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01,
  };

  uint8_t src[16] = {0};
  write_ipv6_src(frame, src);
  write_ipv6_dst(frame, dst_ff02_1);

  frame[PORTS_BYTE_OFFSET] = 0x0B; // ingress=0, egress=0xB

  const uint16_t ingress_mask = (1U << (1 - 1)); // port 1
  const uint16_t all_ports_mask = 0x000F;

  const BmL2PolicyRxResult r = bm_l2_policy_rx_apply(
      frame, sizeof(frame), ingress_mask, all_ports_mask, routing_cb);

  // Neighbor multicast should NOT consult routing_cb (policy keeps behavior aligned with l2.c gate)
  EXPECT_EQ(routing_cb_fake.call_count, 0);

  // No forwarding decision is made here (egress_mask stays 0)
  EXPECT_EQ(r.egress_mask, 0);
  EXPECT_TRUE(r.should_submit);

  // Ingress is still encoded for app-visible buffer
  EXPECT_EQ(ingress_nibble(frame), 1);

  // Egress nibble preserved (no callback path)
  EXPECT_EQ(egress_nibble(frame), 0x0B);
}

TEST_F(L2Policy, LinkLocalNonNeighbor_withNullCallback_doesNotForward_submits) {
  uint8_t frame[MIN_FRAME_LEN] = {0};
  write_ethertype_ipv6(frame);

  uint8_t dst_ff02_2[16] = {
      0xFF, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x02,
  };
  uint8_t src[16] = {0};
  write_ipv6_src(frame, src);
  write_ipv6_dst(frame, dst_ff02_2);

  frame[PORTS_BYTE_OFFSET] = 0x07; // ingress=0, egress=7

  const uint16_t ingress_mask = (1U << (2 - 1)); // port 2
  const uint16_t all_ports_mask = 0x000F;

  const BmL2PolicyRxResult r = bm_l2_policy_rx_apply(
      frame, sizeof(frame), ingress_mask, all_ports_mask, nullptr);

  // No callback => no forward decision
  EXPECT_EQ(r.egress_mask, 0);
  EXPECT_TRUE(r.should_submit);

  // Ingress encoded
  EXPECT_EQ(ingress_nibble(frame), 2);

  // Egress preserved (since callback not run / not cleared)
  EXPECT_EQ(egress_nibble(frame), 7);
}

TEST_F(L2Policy, IngressMaskZero_doesNotMutate_portsNibble_defaultsToSubmit) {
  uint8_t frame[MIN_FRAME_LEN] = {0};
  write_ethertype_ipv6(frame);

  uint8_t dst_ff03_1[16] = {
      0xFF, 0x03, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01,
  };
  uint8_t src[16] = {0};
  write_ipv6_src(frame, src);
  write_ipv6_dst(frame, dst_ff03_1);

  frame[PORTS_BYTE_OFFSET] = 0x9C; // ingress=9, egress=0xC (junk we can detect)

  const BmL2PolicyRxResult r = bm_l2_policy_rx_apply(
      frame, sizeof(frame), 0 /*ingress_mask*/, 0x000F, routing_cb);

  // Should not decode a port number
  EXPECT_EQ(r.ingress_port_num, 0);

  // Should not mutate ports byte
  EXPECT_EQ(frame[PORTS_BYTE_OFFSET], 0x9C);

  // Default behavior is submit=true, no forwarding decision
  EXPECT_TRUE(r.should_submit);
  EXPECT_EQ(r.egress_mask, 0);

  // No callback invoked because ingress is invalid
  EXPECT_EQ(routing_cb_fake.call_count, 0);
}

TEST_F(L2Policy, ShortFrameLen_doesNotCrash_orMutate_orCallCallback) {
  uint8_t frame[MIN_FRAME_LEN] = {0};
  write_ethertype_ipv6(frame);

  // Put a recognizable value in ports byte so we can detect unintended changes
  frame[PORTS_BYTE_OFFSET] = 0x55;

  const uint16_t ingress_mask = (1U << (1 - 1));
  const uint16_t all_ports_mask = 0x000F;

  // Pass a length smaller than required to contain IPv6 dst address
  const size_t short_len = IPV6_DST_OFFSET + 1;

  const BmL2PolicyRxResult r = bm_l2_policy_rx_apply(
      frame, short_len, ingress_mask, all_ports_mask, routing_cb);

  EXPECT_TRUE(r.should_submit);
  EXPECT_EQ(r.egress_mask, 0);
  EXPECT_EQ(r.ingress_port_num, 0);

  // Ports byte should be untouched
  EXPECT_EQ(frame[PORTS_BYTE_OFFSET], 0x55);

  EXPECT_EQ(routing_cb_fake.call_count, 0);
}

TEST_F(L2Policy, PrepareForwardedCopy_clearsIngressOnly_preservesEgress) {
  uint8_t frame[MIN_FRAME_LEN] = {0};
  write_ethertype_ipv6(frame);

  // ingress=3, egress=9
  frame[PORTS_BYTE_OFFSET] = (uint8_t)((3U << 4) | 9U);

  bm_l2_policy_prepare_forwarded_copy(frame, sizeof(frame));

  EXPECT_EQ(ingress_nibble(frame), 0);
  EXPECT_EQ(egress_nibble(frame), 9);
}

TEST_F(L2Policy, PrepareForwardedCopy_isNoopOnShortFrame) {
  uint8_t frame[MIN_FRAME_LEN] = {0};
  write_ethertype_ipv6(frame);
  frame[PORTS_BYTE_OFFSET] = 0xAB;

  const size_t short_len = IPV6_SRC_OFFSET + 1; // shorter than ports byte offset
  bm_l2_policy_prepare_forwarded_copy(frame, short_len);

  EXPECT_EQ(frame[PORTS_BYTE_OFFSET], 0xAB);
}

TEST_F(L2Policy,
       LinkLocalNonNeighbor_callbackCanReturnSubmitTrue_forwardMaskPropagates) {
  // Switch fake implementation for this test
  routing_cb_fake.custom_fake = routing_cb_impl_forward_and_submit;

  uint8_t frame[MIN_FRAME_LEN] = {0};
  write_ethertype_ipv6(frame);

  uint8_t dst_ff02_2[16] = {
      0xFF, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x02,
  };
  uint8_t src[16] = {0};
  write_ipv6_src(frame, src);
  write_ipv6_dst(frame, dst_ff02_2);

  frame[PORTS_BYTE_OFFSET] = 0x0F;

  const uint16_t ingress_mask = (1U << (3 - 1)); // port 3

  const BmL2PolicyRxResult r = bm_l2_policy_rx_apply(
      frame, sizeof(frame), ingress_mask, 0x000F, routing_cb);

  EXPECT_EQ(routing_cb_fake.call_count, 1);
  EXPECT_TRUE(r.should_submit);
  EXPECT_EQ(r.egress_mask, (uint16_t)((1U << (1 - 1)) | (1U << (2 - 1))));

  // Egress nibble cleared after callback
  EXPECT_EQ(egress_nibble(frame), 0);

  // Ingress nibble reflects ingress port
  EXPECT_EQ(ingress_nibble(frame), 3);
}