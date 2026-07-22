#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "bm_service_request.h"
#include "bm_messages_helper.h"
#include "util.h"

void metrics_service_init(void);
bool metrics_service_request(uint64_t target_node_id,
                             BmServiceReplyCb reply_cb, uint32_t timeout_s);

typedef BmErr (*MetricComponentDataCb)(const char *metric_key, const BmEncoderTableEntry_t **lut,
                                       size_t *num_fields);

BmErr metrics_service_add_component(const char *metric_key, MetricComponentDataCb cb,
                                    size_t fields_count);

#ifdef __cplusplus
}
#endif
