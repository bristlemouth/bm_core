#pragma once

#include "messages.h"
#include "network_device.h"
#include "util.h"
#include <stdbool.h>
#include <stdint.h>

// FIXME: Remove when we can split payloads.
#define bcmp_max_payload_size_bytes (1500)
#define ipv6_header_length (40)
// 1500 MTU minus ipv6 header
#define max_payload_len (bcmp_max_payload_size_bytes - ipv6_header_length)

typedef enum {
  BcmpEventRx,
  BcmpEventHeartbeat,
} BcmpQueueType;

typedef struct {
  BcmpQueueType type;
  void *data;
  uint32_t size;
} BcmpQueueItem;

BmErr bcmp_init(NetworkDevice network_device);
void *bcmp_get_queue(void);
BmErr bcmp_tx(const BmIpAddr *dst, BcmpMessageType type, uint8_t *data,
              uint16_t size, uint32_t seq_num,
              BmErr (*reply_cb)(uint8_t *payload));
BmErr bcmp_ll_forward(BcmpHeader *header, void *data, uint32_t size,
                      uint8_t ingress_port);
