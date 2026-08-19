#ifndef __PACKET_H__
#define __PACKET_H__

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
typedef BcmpProcessCb BcmpRequestFullCb;
typedef BmErr (*BcmpRequestPayloadCb)(uint8_t *payload);

typedef struct {
  BcmpRequestFullCb full;
  BcmpRequestPayloadCb payload;
} BcmpSequencedRequestCb;

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

/*!
 @brief Set Reply Handler For A Sequenced Request
                                                                                
 @details Set exactly one member.
          full    - Receives the whole BcmpProcessData (header, src/dst, size).
                    It can identify which request a reply or timeout belongs to
                    as the incoming data will be zeroed out.
          payload - Receives only the payload pointer. A NULL payload means a
                    time out.
          null    - Will use the BcmpProcessCb callback passed into packet_add.
                    Zeroed out incoming data indicates a timedout message
 */
static inline BcmpSequencedRequestCb packet_full_cb(BcmpRequestFullCb cb) {
  return (BcmpSequencedRequestCb){.full = cb, .payload = NULL};
}

static inline BcmpSequencedRequestCb
packet_payload_cb(BcmpRequestPayloadCb cb) {
  return (BcmpSequencedRequestCb){.full = NULL, .payload = cb};
}

static inline BcmpSequencedRequestCb packet_null_cb(void) {
  return (BcmpSequencedRequestCb){.full = NULL, .payload = NULL};
}

#endif
