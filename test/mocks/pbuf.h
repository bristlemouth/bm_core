#ifndef __PBUF_H__
#define __PBUF_H__

#include <stdint.h>
struct pbuf {
  struct pbuf *next;
  void *payload;
  uint16_t tot_len;
  uint16_t len;
  uint8_t type_internal;
  uint8_t flags;
  uint8_t ref;
  uint8_t if_idx;
};

#endif
