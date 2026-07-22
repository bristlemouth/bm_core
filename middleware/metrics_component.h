#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "bm_messages_helper.h"
#include "util.h"

typedef BmErr (*MetricComponentDataCb)(const char *metric_key, const BmEncoderTableEntry_t **lut,
                                       size_t *num_fields);

BmErr metrics_service_add_component(const char *metric_key, MetricComponentDataCb cb,
                                    size_t fields_count);

#ifdef __cplusplus
}
#endif
