#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "bm_service_request.h"

#define CONFIG_CBOR_MAP_PARTITION_ID_SYS 1
#define CONFIG_CBOR_MAP_PARTITION_ID_HW 2
#define CONFIG_CBOR_MAP_PARTITION_ID_USER 3

void config_cbor_map_service_init(void);
bool config_cbor_map_service_request(uint64_t target_node_id,
                                     uint32_t partition_id,
                                     BmServiceReplyCb reply_cb,
                                     uint32_t timeout_s);

#ifdef __cplusplus
}
#endif
