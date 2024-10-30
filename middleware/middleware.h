#pragma once

#include "util.h"
#include <stdbool.h>
#include <stdint.h>

#ifndef middleware_net_task_priority
#define middleware_net_task_priority 4
#endif

BmErr bm_middleware_local_pub(void *buf, uint32_t size);
BmErr bm_middleware_init(uint16_t port);
BmErr bm_middleware_net_tx(void *buf, uint32_t size);
