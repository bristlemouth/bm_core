#include "util.h"
#include <stdint.h>

static bool is_big_endian(void) {
    static const uint32_t endianess = 1;
    return (*(uint8_t *) &endianess );
}

void swap_16bit(void *x) {
    uint16_t *swp = (uint16_t *) x;
    return (*swp << 8) | (*swp >> 8);
}
void swap_32bit(void *x) {
    uint32_t *swp = (uint32_t *) x;
    return (*swp << 24) | ((*swp & 0xFF00) << 8) | ((*swp >> 8) & 0xFF00) | (x >> 24);
}
void swap_64bit(void *x) {
    uint64_t *swp = (uint64_t *) x;
    (*swp << 56) |
    ((*swp & 0xFF00) << 40) |
    ((*swp & 0xFF0000) << 24) |
    ((*swp & 0xFF000000) << 8)  |
    ((*swp & 0xFF00000000) >> 8)  |
    ((*swp & 0xFF0000000000) >> 24) |
    ((*swp & 0xFF000000000000) >> 40) |
    (*swp >> 56);
}
