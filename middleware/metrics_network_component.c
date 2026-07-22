#include "metrics_network_component.h"
#include "bm_adin2111.h"
#include "bm_config.h"
#include "bm_messages_helper.h"
#include "bristlemouth.h"
#include "metrics_component.h"
#include "network_device.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define metrics_network_component_key "network_port_stats"
#define METRICS_NET_MAX_PORTS (2)

typedef struct {
  uint8_t sqi;   // signal quality indicator
  uint16_t mse;  // raw MSE_VAL register
  uint8_t lq;    // adi_phy_LinkQuality_e: 0 poor / 1 marginal / 2 good
  uint16_t rxe;  // frame_check_rx_error_count
  uint16_t sye;  // SYMB_ERR_CNT
  uint16_t fc;   // FALSE_CARRIER_CNT
  uint16_t len;  // LEN_ERR_CNT
  uint16_t algn; // ALGN_ERR_CNT
} NetPortValues;

typedef struct {
  const char *name;
  BmField_t type;
  size_t offset; // location of the value within NetPortValues
} NetFieldDesc;

static const NetFieldDesc net_fields[] = {
    {"sqi", UINT8, offsetof(NetPortValues, sqi)},
    {"mse", UINT16, offsetof(NetPortValues, mse)},
    {"lq", UINT8, offsetof(NetPortValues, lq)},
    {"rxe", UINT16, offsetof(NetPortValues, rxe)},
    {"sye", UINT16, offsetof(NetPortValues, sye)},
    {"fc", UINT16, offsetof(NetPortValues, fc)},
    {"len", UINT16, offsetof(NetPortValues, len)},
    {"algn", UINT16, offsetof(NetPortValues, algn)},
    // {"osz",     UINT16, offsetof(NetPortValues, osz)},
    // {"usz",     UINT16, offsetof(NetPortValues, usz)},
    // {"odd",     UINT16, offsetof(NetPortValues, odd)},
    // {"odd_pre", UINT16, offsetof(NetPortValues, odd_pre)},
};

#define METRICS_NET_FIELDS_PER_PORT (sizeof(net_fields) / sizeof(net_fields[0]))
#define METRICS_NET_MAX_FIELDS (1 + METRICS_NET_MAX_PORTS * METRICS_NET_FIELDS_PER_PORT)

static uint8_t net_num_ports;
static NetPortValues net_values[METRICS_NET_MAX_PORTS];
static char net_keys[METRICS_NET_MAX_FIELDS][max_key_len];
static BmEncoderTableEntry_t net_lut[METRICS_NET_MAX_FIELDS];
static size_t net_num_fields;

static void refresh_port(uint8_t p, const Adin2111PortStats *st) {
  net_values[p].sqi = st->mse_link_quality.sqi;
  net_values[p].mse = st->mse_link_quality.mseVal;
  net_values[p].lq = (uint8_t)st->mse_link_quality.linkQuality;
  net_values[p].rxe = st->frame_check_rx_error_count;
  net_values[p].sye = st->frame_check_error_counters.SYMB_ERR_CNT;
  net_values[p].fc = st->frame_check_error_counters.FALSE_CARRIER_CNT;
  net_values[p].len = st->frame_check_error_counters.LEN_ERR_CNT;
  net_values[p].algn = st->frame_check_error_counters.ALGN_ERR_CNT;
  // net_values[p].osz = st->frame_check_error_counters.OSZ_CNT; ...
}

static BmErr network_component_data(const char *metric_key,
                                    const BmEncoderTableEntry_t **lut,
                                    size_t *num_fields) {
  (void)metric_key;
  NetworkDevice nd = bristlemouth_network_device();
  if (nd.trait == NULL || nd.trait->port_stats == NULL) {
    bm_debug("metrics: no network device / port_stats\n");
    return BmEINVAL;
  }

  for (uint8_t p = 0; p < net_num_ports; p++) {
    Adin2111PortStats st;
    memset(&st, 0, sizeof(st));
    if (nd.trait->port_stats(nd.self, p, &st) != BmOK) {
      bm_debug("metrics: port_stats failed for port %u; reporting zeros\n", p);
      memset(&net_values[p], 0, sizeof(net_values[p]));
      continue;
    }
    refresh_port(p, &st);
  }

  *lut = net_lut;
  *num_fields = net_num_fields;
  return BmOK;
}

void metrics_network_component_init(void) {
  NetworkDevice nd = bristlemouth_network_device();
  uint8_t nports =
      (nd.trait && nd.trait->num_ports) ? nd.trait->num_ports() : 0;
  if (nports > METRICS_NET_MAX_PORTS) {
    nports = METRICS_NET_MAX_PORTS;
  }
  net_num_ports = nports;

  size_t idx = 0;
  net_lut[idx].key = "num_ports"; /* scalar, no port suffix */
  net_lut[idx].type = UINT8;
  net_lut[idx].value_source = &net_num_ports;
  idx++;

  for (uint8_t p = 0; p < nports; p++) {
    uint8_t port_num = p + 1; /* Bristlemouth ports are 1-indexed */
    for (size_t f = 0; f < METRICS_NET_FIELDS_PER_PORT; f++) {
      snprintf(net_keys[idx], max_key_len, "%s_%u", net_fields[f].name,
               port_num);
      net_lut[idx].key = net_keys[idx];
      net_lut[idx].type = net_fields[f].type;
      net_lut[idx].value_source =
          (const uint8_t *)&net_values[p] + net_fields[f].offset;
      idx++;
    }
  }
  net_num_fields = idx;

  metrics_service_add_component(metrics_network_component_key,
                                network_component_data, METRICS_NET_MAX_FIELDS);
}
