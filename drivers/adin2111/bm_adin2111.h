#ifndef __BM_ADIN2111_H__
#define __BM_ADIN2111_H__

#include "adin2111.h"
#include "network_device.h"
#include "util.h"

#define ADIN2111_PORT_MASK (3U)
#define ADIN2111_ETHERTYPE_PTP (0x88F7U)

typedef struct {
  // IEEE 1588 frame (ethertype 0x88F7) received on port_num 1-2, these are not
  // passed to L2. rx_ts is the ingress timestamp, NULL if invalid.
  void (*receive)(uint8_t port_num, const uint8_t *data, size_t length,
                  const adi_mac_TsTimespec_t *rx_ts);
  // Egress timestamp requested with adin2111_ptp_send was captured
  void (*egress_timestamp_ready)(uint8_t port_num,
                                 adi_mac_EgressCapture_e capture);
} Adin2111PtpCallbacks;

typedef struct {
  adi_phy_MseLinkQuality_t mse_link_quality;
  adi_phy_FrameChkErrorCounters_t frame_check_error_counters;
  uint16_t frame_check_rx_error_count;
  uint32_t frame_check_frame_count;
  adi_eth_MacStatCounters_t mac_stats;
  adi_eth_LinkStatus_e link_status;
} Adin2111PortStats;

#ifdef __cplusplus
extern "C" {
#endif

BmErr adin2111_init(void);
NetworkDevice adin2111_network_device(void);
// PTP functions require bm_adin2111_ptp_enabled. Callbacks are called from,
// and the functions below must only be called from, the L2 thread.
BmErr adin2111_ptp_register_callbacks(const Adin2111PtpCallbacks *callbacks);
BmErr adin2111_ptp_send(uint8_t *data, size_t length, uint8_t port_num,
                        adi_mac_EgressCapture_e capture);
BmErr adin2111_ptp_get_egress_timestamp(uint8_t port_num,
                                        adi_mac_EgressCapture_e capture,
                                        adi_mac_TsTimespec_t *ts);

#ifdef __cplusplus
}
#endif

#endif // __BM_ADIN2111_H__
