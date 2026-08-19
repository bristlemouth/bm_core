#include "messages.h"
#include "util.h"
#include <stdint.h>

#define packet_retry_count 3

typedef struct {
  BcmpHeader *header;
  uint8_t *payload;
  uint32_t size;
  BmIpAddr *src;
  BmIpAddr *dst;
  uint8_t ingress_port;
} BcmpProcessData;

typedef void *(*BcmpGetData)(void *payload);
typedef BmIpAddr *(*BcmpGetIPAddr)(void *payload);
typedef void (*BcmpUpdatePacketRef)(void *payload);
typedef BmErr (*BcmpSendPacket)(void *payload);
typedef uint16_t (*BcmpGetChecksum)(void *payload, uint32_t size);
typedef BmErr (*BcmpProcessCb)(BcmpProcessData data);
typedef BmErr (*BcmpSequencedRequestCb)(uint8_t *payload);

typedef struct {
  bool sequenced_reply;
  bool sequenced_request;
  BcmpProcessCb process;
} BcmpPacketCfg;

typedef struct {
  BcmpGetIPAddr src_ip;
  BcmpGetIPAddr dst_ip;
  BcmpGetData data;
  BcmpGetChecksum checksum;
  BcmpUpdatePacketRef increment;
  BcmpUpdatePacketRef decrement;
  BcmpSendPacket send;
} BcmpPacketCb;

BmErr packet_init(BcmpPacketCb cb);
BmErr packet_add(BcmpPacketCfg *cfg, BcmpMessageType type);
uint16_t packet_checksum(void *payload, uint32_t size);
BmErr process_received_message(void *payload, uint32_t size);
BmErr serialize(void *payload, void *data, uint32_t size, BcmpMessageType type,
                uint32_t seq_num, BcmpSequencedRequestCb cb);
BmErr packet_remove(BcmpMessageType type);
