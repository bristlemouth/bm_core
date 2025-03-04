#include "config_cbor_map_service.h"
#include "bm_config.h"
#include "bm_os.h"
#include "bm_service.h"
#include "cbor_service_helper.h"
#include "config_cbor_map_srv_reply_msg.h"
#include "config_cbor_map_srv_request_msg.h"
#include "device.h"
#include <inttypes.h>

#define config_map_suffix "/config_map"

static bool config_map_service_handler(size_t service_strlen,
                                       const char *service, size_t req_data_len,
                                       uint8_t *req_data, size_t *buffer_len,
                                       uint8_t *reply_data) {
  (void)service_strlen;
  (void)service;
  bool rval = false;
  uint8_t *cbor_map = NULL;
  BmConfigPartition config;
  bm_debug("Data received on config map service\n");
  do {
    ConfigCborMapRequestData req;
    if (config_cbor_map_request_decode(&req, req_data, req_data_len) !=
        CborNoError) {
      bm_debug("Failed to decode config map request\n");
      break;
    }
    if (req.partition_id == CONFIG_CBOR_MAP_PARTITION_ID_SYS) {
      config = BM_CFG_PARTITION_SYSTEM;
    } else if (req.partition_id == CONFIG_CBOR_MAP_PARTITION_ID_HW) {
      config = BM_CFG_PARTITION_HARDWARE;
    } else if (req.partition_id == CONFIG_CBOR_MAP_PARTITION_ID_USER) {
      config = BM_CFG_PARTITION_USER;
    } else {
      config = (BmConfigPartition)0xFF;
      bm_debug("Invalid partition id\n");
    }
    size_t buffer_size;
    if (config != (BmConfigPartition)0xFF) {
      cbor_map = services_cbor_as_map(&buffer_size, config);
    }
    ConfigCborMapReplyData reply = {0, 0, 0, 0, NULL};
    reply.node_id = node_id();
    reply.partition_id = req.partition_id;
    reply.cbor_data = cbor_map;
    reply.cbor_encoded_map_len = (reply.cbor_data) ? buffer_size : 0;
    reply.success = (reply.cbor_data) ? true : false;
    size_t encoded_len;
    if (config_cbor_map_reply_encode(&reply, reply_data, *buffer_len,
                                     &encoded_len) != CborNoError) {
      bm_debug("Failed to encode config map reply\n");
      break;
    }
    *buffer_len = encoded_len;
    rval = true;
  } while (0);
  if (cbor_map) {
    bm_free(cbor_map);
  }
  return rval;
}

void config_cbor_map_service_init(void) {
  static char config_map_str[BM_SERVICE_MAX_SERVICE_STRLEN];
  size_t topic_strlen =
      snprintf(config_map_str, sizeof(config_map_str), "%016" PRIx64 "%s",
               node_id(), config_map_suffix);
  if (topic_strlen > 0) {
    bm_service_register(topic_strlen, config_map_str,
                        config_map_service_handler);
  } else {
    bm_debug("Failed to register sys info service\n");
  }
}

bool config_cbor_map_service_request(uint64_t target_node_id,
                                     uint32_t partition_id,
                                     BmServiceReplyCb reply_cb,
                                     uint32_t timeout_s) {
  bool rval = false;
  char *target_service_str = (char *)bm_malloc(BM_SERVICE_MAX_SERVICE_STRLEN);
  if (target_service_str) {
    size_t target_service_strlen =
        snprintf(target_service_str, BM_SERVICE_MAX_SERVICE_STRLEN,
                 "%016" PRIx64 "%s", target_node_id, config_map_suffix);
    ConfigCborMapRequestData req = {partition_id};
    static const size_t cbor_buflen =
        sizeof(ConfigCborMapRequestData) + 25; // Arbitrary padding.
    uint8_t *cbor_buffer = (uint8_t *)bm_malloc(cbor_buflen);
    if (cbor_buffer) {
      size_t req_data_len;
      if (config_cbor_map_request_encode(&req, cbor_buffer, cbor_buflen,
                                         &req_data_len) != CborNoError) {
        bm_debug("Failed to encode config map request\n");
      } else if (target_service_strlen > 0) {
        rval =
            bm_service_request(target_service_strlen, target_service_str,
                               req_data_len, cbor_buffer, reply_cb, timeout_s);
      }
      bm_free(cbor_buffer);
    }
  }
  bm_free(target_service_str);
  return rval;
}
