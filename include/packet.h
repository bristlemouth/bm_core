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
} BcmpProcessData;

typedef void *(*BcmpGetData)(void *payload);
typedef void *(*BcmpGetIPAddr)(void *payload);
typedef uint16_t (*BcmpGetChecksum)(void *payload, uint32_t size);
typedef BmErr (*BcmpProcessCb)(BcmpProcessData data);
typedef BmErr (*BcmpSequencedRequestCb)(uint8_t *payload);

typedef struct {
  uint32_t size;
  bool sequenced_reply;
  bool sequenced_request;
  BcmpProcessCb process;
} BcmpPacketCfg;

BmErr packet_init(BcmpGetIPAddr src_ip, BcmpGetIPAddr dst_ip, BcmpGetData data,
                  BcmpGetChecksum checksum);
BmErr packet_add(BcmpPacketCfg *cfg, BcmpMessageType type);
BmErr process_received_message(void *payload);
BmErr serialize(void *payload, void *data, BcmpMessageType type,
                uint32_t seq_num, BcmpSequencedRequestCb cb);
BmErr packet_remove(BcmpMessageType type);
