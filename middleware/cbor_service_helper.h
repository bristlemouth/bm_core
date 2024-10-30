#include "bm_configs_generic.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t *services_cbor_as_map(size_t *buffer_size, BmConfigPartition type);
uint32_t services_cbor_encoded_as_crc32(BmConfigPartition type);

#ifdef __cplusplus
}
#endif
