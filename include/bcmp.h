#pragma once

#include "messages.h"
#include "util.h"
#include <stdbool.h>
#include <stdint.h>

/* Ingress and Egress ports are mapped to the 5th and 6th byte of the IPv6 src address as per
    the bristlemouth protocol spec */
#define CLEAR_PORTS(x) (x[1] &= (~(0xFFFFU)))

#define IP_PROTO_BCMP (0xBC)
#define bcmp_max_payload_size_bytes                                            \
  (1500) // FIXME: Remove when we can split payloads.
static const uint32_t DEFAULT_MESSAGE_TIMEOUT_MS = 24;

BmErr bcmp_init(void);
BmErr bcmp_tx(const void *dst, BcmpMessageType type, uint8_t *data,
              uint16_t size, uint32_t seq_num,
              BmErr (*reply_cb)(uint8_t *payload));
BmErr bcmp_ll_forward(BcmpHeader *header, void *data, uint32_t size,
                      uint8_t ingress_port);
void bcmp_link_change(uint8_t port, bool state);
