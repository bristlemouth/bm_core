#ifndef __BM_CONFIGS_GENERIC_H__
#define __BM_CONFIGS_GENERIC_H__

#include "configuration.h"
#include <stddef.h>
#include <stdint.h>

bool bm_config_read(BmConfigPartition partition, uint32_t offset,
                    uint8_t *buffer, size_t length, uint32_t timeout_ms);
bool bm_config_write(BmConfigPartition partition, uint32_t offset,
                     uint8_t *buffer, size_t length, uint32_t timeout_ms);
void bm_config_reset(void);

#endif // __BM_CONFIGS_GENERIC_H__
