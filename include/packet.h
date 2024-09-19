#include "messages.h"
#include "util.h"
#include <stdint.h>

typedef struct {
  BcmpHeader *header;
  uint8_t *payload;
  void *src;
  void *dst;
  uint8_t ingress_port;
  uint32_t seq_num;
} BcmpParserData;

typedef void *(*BcmpGetData)(void *payload);
typedef void *(*BcmpGetIPAddr)(void *payload);
typedef uint16_t (*BcmpGetChecksum)(void *payload, uint32_t size);
typedef BmErr (*BcmpParseCb)(BcmpParserData data);
typedef BmErr (*BcmpSequencedRequestCb)(uint8_t *payload);

typedef struct {
  uint32_t size;
  bool sequenced_reply;
  bool sequenced_request;
  BcmpParseCb parse;
} BcmpPacketCfg;

BmErr packet_init(BcmpGetIPAddr src_ip, BcmpGetIPAddr dst_ip, BcmpGetData data,
                  BcmpGetChecksum checksum);
BmErr packet_add(BcmpPacketCfg *cfg, BcmpMessageType type);
BmErr parse(void *payload);
BmErr serialize(void *payload, void *data, BcmpMessageType type,
                uint32_t seq_num, BcmpSequencedRequestCb cb);
BmErr packet_remove(BcmpMessageType type);
