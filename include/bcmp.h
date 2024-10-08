#pragma once

#include "messages.h"
#include "util.h"
#include <stdbool.h>
#include <stdint.h>

/* Ingress and Egress ports are mapped to the 5th and 6th byte of the IPv6 src address as per
    the bristlemouth protocol spec */
#define clear_ports(x) (x[1] &= (~(0xFFFFU)))
#define IpProtoBcmp (0xBC)
// FIXME: Remove when we can split payloads.
#define bcmp_max_payload_size_bytes (1500)
#define ipv6_header_length (40)
// 1500 MTU minus ipv6 header
#define max_payload_len (bcmp_max_payload_size_bytes - ipv6_header_length)
#define IpProtoBcmp (0xBC)

typedef enum {
  BcmpEventRx,
  BcmpEventHeartbeat,
} BcmpQueueType;

typedef struct {
  BcmpQueueType type;
  void *data;
  uint32_t size;
} BcmpQueueItem;

BmErr bcmp_init(void);
void *bcmp_get_queue(void);
BmErr bcmp_tx(const void *dst, BcmpMessageType type, uint8_t *data,
              uint16_t size, uint32_t seq_num,
              BmErr (*reply_cb)(uint8_t *payload));
BmErr bcmp_ll_forward(BcmpHeader *header, void *data, uint32_t size,
                      uint8_t ingress_port);
void bcmp_link_change(uint8_t port, bool state);
