#include "util.h"
#include <stdint.h>

BmErr bcmp_send_ping_request(uint64_t node, const void *addr,
                             const uint8_t *payload, uint16_t payload_len);
BmErr ping_init(void);