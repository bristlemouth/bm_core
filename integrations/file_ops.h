#include "util.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

BmErr bm_file_append(uint64_t target_node_id, const char *file_name,
                     const uint8_t *buff, uint16_t len);

#ifdef __cplusplus
}
#endif
