/**
 * @file l2_frames.h
 * @brief Internal Ethernet + IPv6 frame layout constants shared by l2.c and l2_policy.c.
 *
 * This header is an internal implementation detail of the network module and
 * is not part of the public API.
 */
#pragma once

#include "util.h"
#include <stdint.h>

// ---------------------------------------------------------------------------
// Ethernet header — field sizes (bytes)
// ---------------------------------------------------------------------------
#define ethernet_destination_size_bytes 6
#define ethernet_src_size_bytes 6
#define ethernet_type_size_bytes 2

// ---------------------------------------------------------------------------
// Ethernet header — field offsets
// ---------------------------------------------------------------------------
#define ethernet_destination_offset (0)
#define ethernet_src_offset                                                    \
  (ethernet_destination_offset + ethernet_destination_size_bytes)
#define ethernet_type_offset (ethernet_src_offset + ethernet_src_size_bytes)

// ---------------------------------------------------------------------------
// Ethernet getter
// ---------------------------------------------------------------------------
#define ethernet_get_type(buf)                                                 \
  (uint8_to_uint16((uint8_t *)&(buf)[ethernet_type_offset]))

// ---------------------------------------------------------------------------
// IPv6 header — field sizes (bytes)
// ---------------------------------------------------------------------------
#define ipv6_version_traffic_class_flow_label_size_bytes 4
#define ipv6_payload_length_size_bytes 2
#define ipv6_next_header_size_bytes 1
#define ipv6_hop_limit_size_bytes 1
#define ipv6_source_address_size_bytes 16
#define ipv6_destination_address_size_bytes 16

// ---------------------------------------------------------------------------
// IPv6 header — field offsets (relative to start of Ethernet frame)
// ---------------------------------------------------------------------------
#define ipv6_version_traffic_class_flow_label_offset                           \
  (ethernet_type_offset + ethernet_type_size_bytes)

#define ipv6_payload_length_offset                                             \
  (ipv6_version_traffic_class_flow_label_offset +                              \
   ipv6_version_traffic_class_flow_label_size_bytes)

#define ipv6_next_header_offset                                                \
  (ipv6_payload_length_offset + ipv6_payload_length_size_bytes)

#define ipv6_hop_limit_offset                                                  \
  (ipv6_next_header_offset + ipv6_next_header_size_bytes)

#define ipv6_source_address_offset                                             \
  (ipv6_hop_limit_offset + ipv6_hop_limit_size_bytes)

#define ipv6_destination_address_offset                                        \
  (ipv6_source_address_offset + ipv6_source_address_size_bytes)

/**
 * Byte within the IPv6 source address that carries the ingress (upper nibble)
 * and egress (lower nibble) port numbers per the Bristlemouth port-encoding
 * convention.
 */
#define ipv6_ingress_egress_ports_offset (ipv6_source_address_offset + 2)
