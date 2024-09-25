#include "util.h"
#include <stdint.h>

BmErr bm_ip_init(void *queue);
void bm_ip_rx_cleanup(void *payload);
void *bm_ip_tx_new(const void *dst, uint32_t size);
BmErr bm_ip_tx_perform(void *payload);
void bm_ip_tx_cleanup(void *payload);
