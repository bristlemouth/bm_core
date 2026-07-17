#include "metrics_service.h"
#include "bm_adin2111.h"
#include "bm_config.h"
#include "bm_os.h"
#include "bm_service.h"
#include "bristlemouth.h"
#include "device.h"
#include "metrics_reply_msg.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define metrics_service_suffix "/metrics"

static bool metrics_service_handler(size_t service_strlen, const char *service,
                                    size_t req_data_len, uint8_t *req_data,
                                    size_t *buffer_len, uint8_t *reply_data) {
    (void)req_data;
    (void)req_data_len;
    bool rval = false;
    bm_debug("Data received on service: %.*s\n", (int)service_strlen, service);
    do {
        NetworkDevice nd = bristlemouth_network_device();
        if (nd.trait == NULL || nd.trait->port_stats == NULL) {
            bm_debug("No network device / port_stats for metrics service\n");
            break;
        }

        MetricsReplyData d;
        memset(&d, 0, sizeof(d));
        d.version = METRICS_REPLY_VERSION;
        d.node_id = node_id();
        d.uptime_ms = bm_ticks_to_ms(bm_get_tick_count());

        uint8_t nports = nd.trait->num_ports();
        if (nports > METRICS_MAX_PORTS) {
            nports = METRICS_MAX_PORTS;
        }
        d.num_ports = nports;

        for (uint8_t p = 0; p < nports; p++) {
            Adin2111PortStats st;
            memset(&st, 0, sizeof(st));
            if (nd.trait->port_stats(nd.self, p, &st) != BmOK) {
                bm_debug("port_stats failed for port %u; reporting zeros\n", p);
            }
            d.ports[p].mse_val          = st.mse_link_quality.mseVal;
            d.ports[p].sqi              = st.mse_link_quality.sqi;
            d.ports[p].link_quality     = (uint8_t)st.mse_link_quality.linkQuality;
            d.ports[p].rx_err_count     = st.frame_check_rx_error_count;
            d.ports[p].symbol_err_count = st.frame_check_error_counters.SYMB_ERR_CNT;
        }

        size_t encoded_len;
        if (metrics_reply_encode(&d, reply_data, *buffer_len, &encoded_len) != CborNoError) {
            bm_debug("Failed to encode metrics service reply\n");
            break;
        }
        *buffer_len = encoded_len;
        rval = true;
    } while (0);

    return rval;
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