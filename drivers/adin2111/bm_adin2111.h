#ifndef __BM_ADIN2111_H__
#define __BM_ADIN2111_H__

#include "adin2111.h"
#include "network_device.h"
#include "util.h"

#define ADIN2111_PORT_MASK (3U)

typedef struct {
  adi_phy_MseLinkQuality_t mse_link_quality;
  adi_phy_FrameChkErrorCounters_t frame_check_error_counters;
  uint16_t frame_check_rx_error_count;
} Adin2111PortStats;

#ifdef __cplusplus
extern "C" {
#endif

BmErr adin2111_init(void);
NetworkDevice adin2111_network_device(void);

// TEMPORARY DEBUG for src/apps/adin_init_time_testing - REVERT BEFORE COMMIT.
// See the matching block in bm_adin2111.c.
BmErr adin2111_debug_get_an_status(uint8_t port_num, adi_phy_AnStatus_t *status);

// Raw PHY register read; reg_addr uses ADI's 0xDDRRRR MMD-in-top-byte encoding,
// i.e. the ADDR_* macros from ADIN2111_phy_addr_rdef.h.
BmErr adin2111_debug_phy_read(uint8_t port_num, uint32_t reg_addr, uint16_t *val);

#ifdef __cplusplus
}
#endif

#endif // __BM_ADIN2111_H__
