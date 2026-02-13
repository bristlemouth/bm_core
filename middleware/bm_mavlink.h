#ifndef __BM_MAVLINK_H__
#define __BM_MAVLINK_H__

#include "util.h"
#include <mavlink/common/mavlink.h>
#include <mavlink/minimal/mavlink.h>
#include <mavlink/standard/mavlink.h>

typedef struct {
  void (*rx_cb)(uint64_t node_id, mavlink_message_t *msg,
                mavlink_status_t *status);
  uint32_t msg_id;
} BmMavLinkRxEntry;

#ifdef __cplusplus
extern "C" {
#endif

BmErr bm_mavlink_init(uint8_t sys_id, uint8_t comp_id, BmMavLinkRxEntry *rx_lut,
                      uint16_t rx_lut_len);
BmErr bm_mavlink_transmit(const mavlink_message_t *msg);

uint8_t bm_mavlink_get_sys_id(void);
uint8_t bm_mavlink_get_comp_id(void);

#ifdef __cplusplus
}
#endif

#endif
