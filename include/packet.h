#ifndef __BM_PACKET_H__
#define __BM_PACKET_H__

#include "messages.h"
#include "util.h"
#include <stdint.h>

typedef struct {
  BCMPHeader *header;
  uint8_t *payload;
  void *src;
  void *dst;
  uint8_t ingress_port;
  uint32_t seq_num;
} BCMPParserData;

typedef void *(*BCMPGetData)(void *payload);
typedef void *(*BCMPGetIPAddr)(void *payload);
typedef uint16_t (*BCMPGetChecksum)(void *payload, uint32_t size);

BmErr packet_init(BCMPGetIPAddr src_ip, BCMPGetIPAddr dst_ip, BCMPGetData data,
                  BCMPGetChecksum checksum);
BmErr parse(void *payload);
BmErr serialize(void *payload, void *data, BCMPMessageType type, uint32_t seq_num);

#endif
