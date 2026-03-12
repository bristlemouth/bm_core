// network/l2_policy.c

#include "l2_policy.h"

#include "l2_frames.h" // shared Ethernet + IPv6 frame layout constants
#include "util.h"      // BmIpAddr + is_*_multicast helpers, ethernet_type_ipv6
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Conservative bounds check: ensure we can read/write src/dst addresses and ports byte.
static inline bool frame_has_ipv6_addrs(const uint8_t *frame,
                                        size_t frame_len) {
  const size_t min_len =
      ipv6_destination_address_offset + ipv6_destination_address_size_bytes;
  return frame != NULL && frame_len >= min_len;
}

static inline uint8_t *ports_byte_ptr(uint8_t *frame, size_t frame_len) {
  if (!frame_has_ipv6_addrs(frame, frame_len)) {
    return NULL;
  }
  return &frame[ipv6_ingress_egress_ports_offset];
}

static inline const BmIpAddr *dst_ip_ptr(const uint8_t *frame,
                                         size_t frame_len) {
  if (!frame_has_ipv6_addrs(frame, frame_len)) {
    return NULL;
  }
  return (const BmIpAddr *)&frame[ipv6_destination_address_offset];
}

static inline BmIpAddr *src_ip_ptr(uint8_t *frame, size_t frame_len) {
  if (!frame_has_ipv6_addrs(frame, frame_len)) {
    return NULL;
  }
  return (BmIpAddr *)&frame[ipv6_source_address_offset];
}

// Ingress lives in upper nibble; egress in lower nibble.
static inline void set_ingress_nibble(uint8_t *ports_byte,
                                      uint8_t ingress_port_1_15) {
  // preserve egress (low nibble)
  *ports_byte =
      (uint8_t)((*ports_byte & 0x0F) | ((ingress_port_1_15 & 0x0F) << 4));
}
static inline void clear_ingress_nibble(uint8_t *ports_byte) {
  *ports_byte = (uint8_t)(*ports_byte & 0x0F);
}
static inline void clear_egress_nibble(uint8_t *ports_byte) {
  *ports_byte = (uint8_t)(*ports_byte & 0xF0);
}

BmL2PolicyRxResult bm_l2_policy_rx_apply(uint8_t *frame, size_t frame_len,
                                         uint16_t ingress_port_mask,
                                         uint16_t all_ports_mask,
                                         L2LinkLocalRoutingCb routing_cb) {
  BmL2PolicyRxResult policy_result = {
      .should_submit = true,
      .egress_mask = 0,
      .ingress_port_num = 0,
  };

  if (!frame_has_ipv6_addrs(frame, frame_len)) {
    // Frame too short to contain IPv6 src/dst addresses. Policy doesn't act.
    return policy_result;
  }

  // Only process IPv6 frames. Non-IPv6 frames (e.g. ARP) must not have their
  // payload bytes misinterpreted as IPv6 addresses or mutated.
  if (ethernet_get_type(frame) != ethernet_type_ipv6) {
    return policy_result;
  }

  // Decode ingress port number 1-15 from mask using ffs.
  // __builtin_ffs returns 1-based index of the lowest set bit, matching port numbering 1-15.
  // If ingress_port_mask is 0, treat as unknown and don't mutate.
  const uint8_t ingress_port_num =
      (uint8_t)__builtin_ffs((unsigned)ingress_port_mask);
  if (ingress_port_num == 0) {
    return policy_result;
  }
  policy_result.ingress_port_num = ingress_port_num;

  // Encode ingress port into the source address upper nibble (application-visible).
  uint8_t *pb = ports_byte_ptr(frame, frame_len);
  if (pb) {
    set_ingress_nibble(pb, ingress_port_num);
  }

  const BmIpAddr *dst_ip = dst_ip_ptr(frame, frame_len);
  if (!dst_ip) {
    return policy_result;
  }

  // Routing decision: global multicast floods all ports except ingress.
  if (is_global_multicast(dst_ip)) {
    policy_result.egress_mask =
        (uint16_t)(all_ports_mask & (uint16_t)~ingress_port_mask);
    // policy_result.should_submit remains true
    return policy_result;
  }

  // For link-local multicast that is NOT ff02::1, consult routing_cb if present.
  if (routing_cb && !is_link_local_neighbor_multicast(dst_ip)) {
    BmIpAddr *src_ip = src_ip_ptr(frame, frame_len);
    uint16_t egress = 0;

    // Callback decides forwarding mask + whether to submit.
    policy_result.should_submit =
        routing_cb(ingress_port_num, &egress, src_ip, dst_ip);
    policy_result.egress_mask = egress;

    // Clear egress nibble after callback (on-wire egress must be zero in the submitted copy).
    if (pb) {
      clear_egress_nibble(pb);
    }
  }

  return policy_result;
}

void bm_l2_policy_prepare_forwarded_copy(uint8_t *frame, size_t frame_len) {
  uint8_t *pb = ports_byte_ptr(frame, frame_len);
  if (!pb) {
    return;
  }

  // Forwarded copies go on-wire: ingress nibble must be zero.
  // Applied only to the forwarded copy, leaving the original buffer's ingress info intact.
  clear_ingress_nibble(pb);
}