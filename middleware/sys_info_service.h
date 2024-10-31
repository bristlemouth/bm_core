#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "bm_service_request.h"

void sys_info_service_init(void);
bool sys_info_service_request(uint64_t target_node_id,
                              BmServiceReplyCb reply_cb, uint32_t timeout_s);

#ifdef __cplusplus
}
#endif
