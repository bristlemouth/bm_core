#ifndef __BM_NETWORK_GENERIC_H__
#define __BM_NETWORK_GENERIC_H__

#include "util.h"

typedef BmErr (*BmIntCb)(void);

void bm_network_register_int_callback(BmIntCb cb);
BmErr bm_network_spi_read_write(uint8_t *tx_buf, uint8_t *rx_buf, uint32_t size,
                                bool use_dma);

#endif //__BM_NETWORK_GENERIC_H__
