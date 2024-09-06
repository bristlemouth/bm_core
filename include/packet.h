#ifndef __BM_PACKET_H__
#define __BM_PACKET_H__

#include "ip_addr.h"
#include "messages.h"
#include "pbuf.h"
#include "util.h"
#include <stdint.h>

typedef struct {
  BCMPHeader *header;
  uint8_t *payload;
  struct pbuf *pbuf;
  ip_addr_t *src;
  ip_addr_t *dst;
  uint8_t ingress_port;
} BCMPParserData;

typedef BMErr (*BCMPParseCb)(BCMPParserData data);

typedef struct {
  BCMPMessageType type;
  BCMPParseCb cb;
} BCMPParserItem;

typedef struct {
  struct pbuf *pbuf;
  ip_addr_t *src;
  ip_addr_t *dst;
} BCMPParseIngest;

BMErr parse(BCMPParseIngest info, BCMPParserItem *items, uint32_t size);
BMErr serialize(void *buf, BCMPMessageType type);

#endif
