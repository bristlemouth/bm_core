#pragma once

#include "util.h"
#include <stdbool.h>
#include <stdint.h>

#ifndef middleware_net_task_priority
#define middleware_net_task_priority 4
#endif

typedef void (*BmMiddlewareRxCb)(uint64_t node_id, void *buf, uint32_t size);
typedef bool (*BmMiddlewareRoutingCb)(uint8_t ingress_port,
                                      uint16_t *egress_ports, BmIpAddr *src);

BmErr bm_middleware_rx(uint16_t port, void *buf, uint64_t node_id,
                       uint32_t size);
BmErr bm_middleware_init(void);
BmErr bm_middleware_add_application(uint16_t port, BmIpAddr dest,
                                    BmMiddlewareRxCb rx_cb,
                                    BmMiddlewareRoutingCb routing_cb);
BmErr bm_middleware_net_tx(uint16_t port, void *buf, uint32_t size);
