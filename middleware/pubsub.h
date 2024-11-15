#pragma once

#include "util.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BM_COMMON_PUB_SUB_VERSION
#define BM_COMMON_PUB_SUB_VERSION (2)
#endif // BM_COMMON_PUB_SUB_VERSION

#define BM_TOPIC_MAX_LEN (255)

// Add data structures published through pub sub must have this header.
typedef struct {
  uint8_t type;       // Type of data.
  uint8_t version;    // Protocol Version.
  uint8_t payload[0]; // Payload
} __attribute__((packed)) BmPubSubHeader;

typedef struct {
  uint8_t type;
  uint8_t flags;
  uint8_t topic_len;
  BmPubSubHeader ext_header;
  const char topic[0];
} __attribute__((packed)) BmPubSubData;

typedef void (*BmPubSubCb)(uint64_t node_id, const char *topic,
                           uint16_t topic_len, const uint8_t *data,
                           uint16_t data_len, uint8_t type, uint8_t version);

BmErr bm_pub(const char *topic, const void *data, uint16_t len, uint8_t type,
             uint8_t version);
BmErr bm_pub_wl(const char *topic, uint16_t topic_len, const void *data,
                uint16_t len, uint8_t type, uint8_t version);
BmErr bm_sub(const char *topic, const BmPubSubCb callback);
BmErr bm_sub_wl(const char *topic, uint16_t topic_len,
                const BmPubSubCb callback);
BmErr bm_unsub(const char *topic, const BmPubSubCb callback);
BmErr bm_unsub_wl(const char *topic, uint16_t topic_len,
                  const BmPubSubCb callback);
void bm_handle_msg(uint64_t node_id, void *buf, uint32_t size);
void bm_print_subs(void);
char *bm_get_subs(void);

#ifdef __cplusplus
}
#endif
