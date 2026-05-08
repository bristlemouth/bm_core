#pragma once

#include "util.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 @brief Handle Routing Of Link-Local Packets

 @details Per Bristlemouth specification in section 5.4.4.3, this is used to
          route Bristlemouth packets to other ports and determine if packets
          need to be handled beyond L2.
          If egress_mask is 0 when this function returns, the packet is not
          forwarded.
          If the callback function returns true, the packet is sent up the
          Bristlemouth stack and handled by the specific application bound
          to the destination IP address

 @param ingress_port Port packet was received on
 @param egress_mask Mask of ports the packet should be forwarded to,
 @param src Source IP address of packet
 @param dest Destination IP address of packet

 @return true if the packet should be submitted through the
              ip stack to the targeted application
         false if the packet should not be handled and
               just forwarded
 */
typedef bool (*L2LinkLocalRoutingCb)(uint8_t ingress_port,
                                     uint16_t *egress_mask, BmIpAddr *src,
                                     const BmIpAddr *dest);

typedef struct {
  // Whether the original RX frame (mutated in-place) should be submitted upward.
  bool should_submit;

  // Port mask for forwarding (0 => don't forward).
  uint16_t egress_mask;

  // Ingress port number decoded from ingress_port_mask via ffs (1-15, or 0 if invalid).
  uint8_t ingress_port_num;
} BmL2PolicyRxResult;

/**
 * Apply L2 RX policy to an Ethernet+IPv6 frame in place:
 *   - encodes ingress port number (1-15) into src IPv6 address nibble
 *   - computes forwarding mask based on:
 *       * global multicast => forward to all_ports_mask & ~ingress_port_mask
 *       * link-local non-neighbor => consult routing_cb if provided
 *   - if routing_cb is used, clears the egress nibble in the src addr after callback
 *
 * Only acts on frames with EtherType 0x86DD (IPv6) that are long enough to
 * contain src and dst addresses. All other frames pass through unchanged.
 *
 * This function is intended to be unit-tested directly (no RTOS, no queues, no alloc).
 */
BmL2PolicyRxResult bm_l2_policy_rx_apply(uint8_t *frame, size_t frame_len,
                                         uint16_t ingress_port_mask,
                                         uint16_t all_ports_mask,
                                         L2LinkLocalRoutingCb routing_cb);

/**
 * Prepare a forwarded *copy* for on-wire transmission:
 *   - clears ingress nibble (on-wire ingress bits must be zero)
 *   - leaves egress nibble intact
 *
 * Call this only on the forwarded copy, not the original RX buffer.
 */
void bm_l2_policy_prepare_forwarded_copy(uint8_t *frame, size_t frame_len);

#ifdef __cplusplus
}
#endif