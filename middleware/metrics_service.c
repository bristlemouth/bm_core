#include "metrics_service.h"
#include "bm_config.h"
#include "bm_os.h"
#include "bm_service.h"
#include "device.h"
#include "ll.h"
#include "metrics_reply_msg.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define metrics_service_suffix "/metrics"

typedef struct {
  const char *key;
  MetricComponentDataCb cb;
  size_t fields_count;
} MetricComponentEntry;

static LL metrics_components;
static uint32_t metrics_component_count;

typedef struct {
  MetricsComponent *components;
  size_t capacity;
  size_t count;
} MetricsCollectCtx;


static BmErr metrics_collect_component(void *data, void *arg) {
  MetricComponentEntry *entry = (MetricComponentEntry *)data;
  MetricsCollectCtx *ctx = (MetricsCollectCtx *)arg;

  if (ctx->count >= ctx->capacity) {
    return BmOK;
  }

  const BmEncoderTableEntry_t *lut = NULL;
  size_t num_fields = 0;
  if (entry->cb(entry->key, &lut, &num_fields) != BmOK || lut == NULL) {
    bm_debug("metrics: component %s produced no data\n", entry->key);
    return BmOK; /* skip this component, keep the rest */
  }

  ctx->components[ctx->count].key = entry->key;
  ctx->components[ctx->count].fields = lut;
  ctx->components[ctx->count].num_fields = num_fields;
  ctx->count++;
  return BmOK;
}

static bool metrics_service_handler(size_t service_strlen, const char *service,
                                    size_t req_data_len, uint8_t *req_data,
                                    size_t *buffer_len, uint8_t *reply_data) {
    (void)req_data;
    (void)req_data_len;
    bool rval = false;
    MetricsComponent *components = NULL;
    bm_debug("Data received on service: %.*s\n", (int)service_strlen, service);
    do {
        MetricsCollectCtx ctx;
        memset(&ctx, 0, sizeof(ctx));
        if (metrics_component_count > 0) {
          components = (MetricsComponent *)bm_malloc(metrics_component_count *
                                                    sizeof(*components));
          if (components == NULL) {
            bm_debug("metrics: failed to allocate component list\n");
            break;
          }
        }
        ctx.components = components;
        ctx.capacity = metrics_component_count;
        ll_traverse(&metrics_components, metrics_collect_component, &ctx);

        MetricsReplyData d;
        memset(&d, 0, sizeof(d));
        d.version = METRICS_REPLY_VERSION;
        d.node_id = node_id();
        d.uptime_ms = bm_ticks_to_ms(bm_get_tick_count());
        d.components = ctx.components;
        d.num_components = ctx.count;

        size_t encoded_len;
        if (metrics_reply_encode(&d, reply_data, *buffer_len, &encoded_len) != CborNoError) {
          bm_debug("Failed to encode metrics service reply\n");
          break;
        }
        *buffer_len = encoded_len;
        rval = true;
      } while (0);

      if (components != NULL) {
        bm_free(components);
      }
      return rval;
}


BmErr metrics_service_add_component(const char *metric_key, MetricComponentDataCb cb,
                                    size_t fields_count) {
  BmErr err = BmEINVAL;
  if (metric_key && cb) {
    MetricComponentEntry entry = {metric_key, cb, fields_count};
    LLItem *item = ll_create_item(NULL, &entry, sizeof(entry), metrics_component_count);
    err = item ? ll_item_add(&metrics_components, item) : BmENOMEM;
    if (err == BmOK) {
      metrics_component_count++;
    }
  }
  return err;
}

void metrics_service_init(void) {
    static char metrics_service_str[BM_SERVICE_MAX_SERVICE_STRLEN];
    size_t topic_strlen =
        snprintf(metrics_service_str, sizeof(metrics_service_str),
                "%016" PRIx64 "%s", node_id(), metrics_service_suffix);
    if (topic_strlen > 0) {
        bm_service_register(topic_strlen, metrics_service_str, metrics_service_handler);
    } else {
        bm_debug("Failed to register metrics service\n");
    }
}

bool metrics_service_request(uint64_t target_node_id,
                             BmServiceReplyCb reply_cb, uint32_t timeout_s) {
    bool rval = false;
    char *target_service_str = (char *)bm_malloc(BM_SERVICE_MAX_SERVICE_STRLEN);
    if (target_service_str) {
        size_t target_service_strlen =
            snprintf(target_service_str, BM_SERVICE_MAX_SERVICE_STRLEN,
                    "%016" PRIx64 "%s", target_node_id, metrics_service_suffix);
        if (target_service_strlen > 0) {
        rval = bm_service_request(target_service_strlen, target_service_str, 0,
                                    NULL, reply_cb, timeout_s);
        }
    }
    bm_free(target_service_str);
    return rval;
}
