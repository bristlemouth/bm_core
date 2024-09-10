#include "util.h"
#include <stdint.h>

bool is_little_endian(void) {
  static const uint32_t endianness = 1;
  return (*(uint8_t *)&endianness);
}

void swap_16bit(void *x) {
  uint16_t *swp = (uint16_t *)x;
  *swp = (*swp << 8) | (*swp >> 8);
}
void swap_32bit(void *x) {
  uint32_t *swp = (uint32_t *)x;
  *swp = (*swp << 24) | ((*swp & 0xFF00) << 8) | ((*swp >> 8) & 0xFF00) | (*swp >> 24);
}
void swap_64bit(void *x) {
  uint64_t *swp = (uint64_t *)x;
  *swp = (*swp << 56) | ((*swp & 0x0000FF00) << 40) | ((*swp & 0x00FF0000) << 24) |
         ((*swp & 0xFF000000) << 8) | ((*swp >> 8) & 0xFF000000) | ((*swp >> 24) & 0x00FF0000) |
         ((*swp >> 40) & 0x0000FF00) | (*swp >> 56);
}
