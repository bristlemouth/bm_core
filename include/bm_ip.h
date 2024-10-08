#include "util.h"
#include <stdint.h>

BmErr bm_ip_init(void);
void *bm_l2_new(uint32_t size);
void *bm_l2_get_payload(void *buf);
void bm_l2_tx_prep(void *buf, uint32_t size);
void bm_l2_free(void *buf);
BmErr bm_l2_submit(void *buf, uint32_t size);
BmErr bm_l2_set_netif(bool up);
const char *bm_ip_get_str(uint8_t idx);
void bm_ip_rx_cleanup(void *payload);
void *bm_ip_tx_new(const void *dst, uint32_t size);
BmErr bm_ip_tx_copy(void *payload, const void *data, uint32_t size,
                    uint32_t offset);
BmErr bm_ip_tx_perform(void *payload, const void *dst);
void bm_ip_tx_cleanup(void *payload);
