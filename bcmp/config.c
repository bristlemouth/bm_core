#include "messages/config.h"
#include "bcmp.h"
#include "bm_config.h"
#include "bm_configs_generic.h"
#include "bm_os.h"
#include "cbor.h"
#include "device.h"
#include "packet.h"
#include "util.h"
#include <inttypes.h>
#include <string.h>

/*!
 @brief Decode Incoming Value Message

 @details Incoming BcmpConfigValueMessage messages can be decoded utilizing
          this API. The incoming data type must be known prior to receiving the
          data and invoking this function.

 @param type Configuration data type
 @param data Data to decode
 @param data_length Length of data
 @param buf Buffer to hold the decoded data
 @param buf_length Length of buffer

 @return 
 */
BmErr bcmp_config_decode_value(ConfigDataTypes type, uint8_t *data,
                               uint32_t data_length, void *buf,
                               size_t *buf_length) {
  BmErr err = BmEINVAL;
  CborValue it;
  CborParser parser;
  ConfigDataTypes type_cmp;

  // Check that variables exist
  if (!data || !data_length || !buf || !buf_length || !*buf_length) {
    return err;
  }

  err = BmEBADMSG;

  if (cbor_parser_init(data, data_length, 0, &parser, &it) == CborNoError &&
      cbor_value_is_valid(&it) && cbor_type_to_config(&it, &type_cmp)) {
    // Ensure that the type expected is what is actually being obtained
    if (type_cmp != type) {
      err = BmEIO;
    } else {
      switch (type) {
      case UINT32: {
        uint64_t temp = 0;
        uint32_t *p = (uint32_t *)buf;
        if (*buf_length >= sizeof(uint32_t) &&
            cbor_value_get_uint64(&it, &temp) == CborNoError) {
          *p = (uint32_t)temp;
          err = BmOK;
        }
      } break;
      case INT32: {
        int64_t temp = 0;
        int32_t *p = (int32_t *)buf;
        if (*buf_length >= sizeof(int32_t) &&
            cbor_value_get_int64(&it, &temp) == CborNoError) {
          *p = (int32_t)temp;
          err = BmOK;
        }
      } break;
      case FLOAT: {
        float *p = (float *)buf;
        if (*buf_length >= sizeof(float) &&
            cbor_value_get_float(&it, p) == CborNoError) {
          err = BmOK;
        }
      } break;
      case STR: {
        char *p = (char *)buf;
        uint32_t init_length = *buf_length;
        if (cbor_value_copy_text_string(&it, p, buf_length, NULL) ==
            CborNoError) {
          if (*buf_length > init_length) {
            break;
          }
          p[*buf_length - 1] = '\0';
          err = BmOK;
        }
      } break;
      case BYTES: {
        if (cbor_value_copy_byte_string(&it, (uint8_t *)buf,
                                        (size_t *)buf_length,
                                        NULL) == CborNoError) {
          err = BmOK;
        }
      } break;
      case ARRAY: {
        err = BmOK;
      } break;
      }
    }
  }

  return err;
}

/*!
 @brief Get a configuration value

 @details Get a configuration value from a target node, in a selected partition
          by key name

 @param target_node_id Target node id to obtain config value
 @param partition Partition to obtain configuration from
 @param key_len Length of the key name
 @param key Pointer to the key name string
 @param err Error value passed in as a pointer
 @param reply_cb Callback when a reply is received over the bus, can be NULL

 @return true if get message is sent properly
 @return false otherwise
 */
bool bcmp_config_get(uint64_t target_node_id, BmConfigPartition partition,
                     size_t key_len, const char *key, BmErr *err,
                     BmErr (*reply_cb)(uint8_t *)) {
  bool rval = false;
  if (key) {
    *err = BmEINVAL;
    size_t msg_size = sizeof(BmConfigGet) + key_len;
    BmConfigGet *get_msg = (BmConfigGet *)bm_malloc(msg_size);
    if (get_msg) {
      do {
        get_msg->header.target_node_id = target_node_id;
        get_msg->header.source_node_id = node_id();
        get_msg->partition = partition;
        if (key_len > MAX_KEY_LEN_BYTES) {
          break;
        }
        get_msg->key_length = key_len;
        memcpy(get_msg->key, key, key_len);
        *err = bcmp_tx(&multicast_ll_addr, BcmpConfigGetMessage,
                       (uint8_t *)get_msg, msg_size, 0, reply_cb);
        if (*err == BmOK) {
          rval = true;
        }
      } while (0);
      bm_free(get_msg);
    }
  }
  return rval;
}

/*!
 @brief Set a configuration value

 @details Set a configuration value on a target node, in a selected partition
          by key name

 @param target_node_id Target node id to set config value
 @param partition Partition to set configuration
 @param key_len Length of the key name
 @param key Pointer to the key name string
 @param err Error value passed in as a pointer
 @param reply_cb Callback when a reply is received over the bus, can be NULL

 @return true if set message is sent properly
 @return false otherwise
 */
bool bcmp_config_set(uint64_t target_node_id, BmConfigPartition partition,
                     size_t key_len, const char *key, size_t value_size,
                     void *val, BmErr *err, BmErr (*reply_cb)(uint8_t *)) {
  bool rval = false;
  if (key) {
    *err = BmEINVAL;
    size_t msg_len = sizeof(BmConfigSet) + key_len + value_size;
    BmConfigSet *set_msg = (BmConfigSet *)bm_malloc(msg_len);
    if (set_msg) {
      do {
        set_msg->header.target_node_id = target_node_id;
        set_msg->header.source_node_id = node_id();
        set_msg->partition = partition;
        if (key_len > MAX_KEY_LEN_BYTES) {
          break;
        }
        set_msg->key_length = key_len;
        memcpy(set_msg->keyAndData, key, key_len);
        set_msg->data_length = value_size;
        memcpy(&set_msg->keyAndData[key_len], val, value_size);
        *err = bcmp_tx(&multicast_ll_addr, BcmpConfigSetMessage,
                       (uint8_t *)set_msg, msg_len, 0, reply_cb);
        if (*err == BmOK) {
          rval = true;
        }
      } while (0);
      bm_free(set_msg);
    }
  }
  return rval;
}

/*!
 @brief Commit a configuration partition

 @details Commit a configuration partition on a target node, this saves any
          updated configuration values and resets the device

 @param target_node_id Target node id to commit the configuration partition to
 @param partition Partition to commit
 @param err Error value passed in as a pointer

 @return true if commit message is sent properly
 @return false otherwise
 */
bool bcmp_config_commit(uint64_t target_node_id, BmConfigPartition partition,
                        BmErr *err) {
  bool rval = false;
  *err = BmEINVAL;
  BmConfigCommit *commit_msg =
      (BmConfigCommit *)bm_malloc(sizeof(BmConfigCommit));
  if (commit_msg) {
    commit_msg->header.target_node_id = target_node_id;
    commit_msg->header.source_node_id = node_id();
    commit_msg->partition = partition;
    *err = bcmp_tx(&multicast_ll_addr, BcmpConfigCommitMessage,
                   (uint8_t *)commit_msg, sizeof(BmConfigCommit), 0, NULL);
    if (*err == BmOK) {
      rval = true;
    }
    bm_free(commit_msg);
  }
  return rval;
}

/*!
 @brief Configuration partition status

 @details Get the all of the key names from a target node's selected partition

 @param target_node_id Target node id to obtaion the key names from
 @param partition Configuration partition to obtain the key names from
 @param err Error value passed in as a pointer
 @param reply_cb Callback when a reply is received over the bus, can be NULL

 @return true if status message is sent properly, false otherwise
 */
bool bcmp_config_status_request(uint64_t target_node_id,
                                BmConfigPartition partition, BmErr *err,
                                BmErr (*reply_cb)(uint8_t *)) {
  bool rval = false;
  *err = BmEINVAL;
  BmConfigStatusRequest *status_req_msg =
      (BmConfigStatusRequest *)bm_malloc(sizeof(BmConfigStatusRequest));
  if (status_req_msg) {
    status_req_msg->header.target_node_id = target_node_id;
    status_req_msg->header.source_node_id = node_id();
    status_req_msg->partition = partition;
    *err = bcmp_tx(&multicast_ll_addr, BcmpConfigStatusRequestMessage,
                   (uint8_t *)status_req_msg, sizeof(BmConfigStatusRequest), 0,
                   reply_cb);
    if (*err == BmOK) {
      rval = true;
    }
    bm_free(status_req_msg);
  }
  return rval;
}

/*!
 @brief Delete key from partition

 @details Delete a key from a target node's configuration partition

 @param target_node_id Target node id to delete config value
 @param partition Partition to delete configuration value from
 @param key_len Length of the key name
 @param key Pointer to the key name string
 @param reply_cb Callback when a reply is received over the bus, can be NULL

 @return true if delete message is sent properly, false otherwise
 */
bool bcmp_config_del_key(uint64_t target_node_id, BmConfigPartition partition,
                         size_t key_len, const char *key,
                         BmErr (*reply_cb)(uint8_t *)) {
  bool rval = false;
  if (key) {
    size_t msg_size = sizeof(BmConfigDeleteKeyRequest) + key_len;
    BmConfigDeleteKeyRequest *del_msg =
        (BmConfigDeleteKeyRequest *)bm_malloc(msg_size);
    if (del_msg) {
      del_msg->header.target_node_id = target_node_id;
      del_msg->header.source_node_id = node_id();
      del_msg->partition = partition;
      del_msg->key_length = key_len;
      memcpy(del_msg->key, key, key_len);
      BmErr err = bcmp_tx(&multicast_ll_addr, BcmpConfigDeleteRequestMessage,
                          (uint8_t *)(del_msg), msg_size, 0, reply_cb);
      rval = (err == BmOK);
      bm_free(del_msg);
    }
  }
  return rval;
}

static bool bcmp_config_status_response(uint64_t target_node_id,
                                        BmConfigPartition partition,
                                        bool commited, uint8_t num_keys,
                                        const ConfigKey *keys, BmErr *err,
                                        uint16_t seq_num) {
  bool rval = false;
  *err = BmEINVAL;
  size_t msg_size = sizeof(BmConfigStatusResponse);
  for (int i = 0; i < num_keys; i++) {
    msg_size += sizeof(BmConfigStatusKeyData);
    msg_size += keys[i].key_len;
  }
  if (msg_size > bcmp_max_payload_size_bytes) {
    return false;
  }
  BmConfigStatusResponse *status_resp_msg =
      (BmConfigStatusResponse *)bm_malloc(msg_size);
  if (status_resp_msg) {
    do {
      status_resp_msg->header.target_node_id = target_node_id;
      status_resp_msg->header.source_node_id = node_id();
      status_resp_msg->committed = commited;
      status_resp_msg->partition = partition;
      status_resp_msg->num_keys = num_keys;
      BmConfigStatusKeyData *key_data =
          (BmConfigStatusKeyData *)status_resp_msg->keyData;
      for (int i = 0; i < num_keys; i++) {
        key_data->key_length = keys[i].key_len;
        memcpy(key_data->key, keys[i].key_buf, keys[i].key_len);
        key_data += sizeof(BmConfigStatusKeyData);
        key_data += keys[i].key_len;
      }
      *err = bcmp_tx(&multicast_ll_addr, BcmpConfigStatusResponseMessage,
                     (uint8_t *)status_resp_msg, msg_size, seq_num, NULL);
      if (*err == BmOK) {
        rval = true;
      }
    } while (0);
    bm_free(status_resp_msg);
  }
  return rval;
}

static void bcmp_config_process_commit_msg(BmConfigCommit *msg) {
  if (msg) {
    save_config((BmConfigPartition)msg->partition, true);
  }
}

static void bcmp_config_process_status_request_msg(BmConfigStatusRequest *msg,
                                                   uint16_t seq_num) {
  if (msg) {
    BmErr err;
    do {
      uint8_t num_keys;
      const ConfigKey *keys = get_stored_keys(msg->partition, &num_keys);
      bcmp_config_status_response(msg->header.source_node_id, msg->partition,
                                  needs_commit(msg->partition), num_keys, keys,
                                  &err, seq_num);
      if (err != BmOK) {
        bm_debug("Error processing config status request.\n");
      }
    } while (0);
  }
}

static bool bcmp_config_send_value(uint64_t target_node_id,
                                   BmConfigPartition partition,
                                   uint32_t data_length, void *data, BmErr *err,
                                   uint16_t seq_num) {
  bool rval = false;
  *err = BmEINVAL;
  size_t msg_len = data_length + sizeof(BmConfigValue);
  BmConfigValue *value_msg = (BmConfigValue *)bm_malloc(msg_len);
  if (value_msg) {
    do {
      value_msg->header.target_node_id = target_node_id;
      value_msg->header.source_node_id = node_id();
      value_msg->partition = partition;
      value_msg->data_length = data_length;
      memcpy(value_msg->data, data, data_length);
      *err = bcmp_tx(&multicast_ll_addr, BcmpConfigValueMessage,
                     (uint8_t *)value_msg, msg_len, seq_num, NULL);
      if (*err == BmOK) {
        rval = true;
      }
    } while (0);
    bm_free(value_msg);
  }
  return rval;
}

static void bcmp_config_process_config_get_msg(BmConfigGet *msg,
                                               uint16_t seq_num) {
  if (msg) {
    do {
      size_t buffer_len = MAX_CONFIG_BUFFER_SIZE_BYTES;
      uint8_t *buffer = (uint8_t *)bm_malloc(buffer_len);
      if (buffer) {
        if (get_config_cbor(msg->partition, msg->key, msg->key_length, buffer,
                            &buffer_len)) {
          BmErr err;
          bcmp_config_send_value(msg->header.source_node_id, msg->partition,
                                 buffer_len, buffer, &err, seq_num);
        }
        bm_free(buffer);
      }
    } while (0);
  }
}

static void bcmp_config_process_config_set_msg(BmConfigSet *msg,
                                               uint16_t seq_num) {
  if (msg) {
    do {
      if (msg->data_length > MAX_CONFIG_BUFFER_SIZE_BYTES ||
          msg->data_length == 0) {
        break;
      }
      if (set_config_cbor(msg->partition, (const char *)msg->keyAndData,
                          msg->key_length, &msg->keyAndData[msg->key_length],
                          msg->data_length)) {
        BmErr err;
        bcmp_config_send_value(
            msg->header.source_node_id, msg->partition, msg->data_length,
            &msg->keyAndData[msg->key_length], &err, seq_num);
      }
    } while (0);
  }
}

static void bcmp_process_value_message(BmConfigValue *msg) {
  CborValue it;
  CborParser parser;
  do {
    if (cbor_parser_init(msg->data, msg->data_length, 0, &parser, &it) !=
        CborNoError) {
      break;
    }
    if (!cbor_value_is_valid(&it)) {
      break;
    }
    ConfigDataTypes type;
    if (!cbor_type_to_config(&it, &type)) {
      break;
    }
    switch (type) {
    case UINT32: {
      uint64_t temp;
      if (cbor_value_get_uint64(&it, &temp) != CborNoError) {
        break;
      }
      bm_debug("Node Id: %016" PRIx64 " Value:%" PRIu32 "\n",
               msg->header.source_node_id, (uint32_t)temp);
      break;
    }
    case INT32: {
      int64_t temp;
      if (cbor_value_get_int64(&it, &temp) != CborNoError) {
        break;
      }
      bm_debug("Node Id: %016" PRIx64 " Value:%" PRId32 "\n",
               msg->header.source_node_id, (int32_t)temp);
      break;
    }
    case FLOAT: {
      float temp;
      if (cbor_value_get_float(&it, &temp) != CborNoError) {
        break;
      }
      bm_debug("Node Id: %016" PRIx64 " Value:%f\n", msg->header.source_node_id,
               temp);
      break;
    }
    case STR: {
      size_t buffer_len = MAX_CONFIG_BUFFER_SIZE_BYTES;
      char *buffer = (char *)bm_malloc(buffer_len);
      if (buffer) {
        do {
          if (cbor_value_copy_text_string(&it, buffer, &buffer_len, NULL) !=
              CborNoError) {
            break;
          }
          if (buffer_len >= MAX_CONFIG_BUFFER_SIZE_BYTES) {
            break;
          }
          buffer[buffer_len] = '\0';
          bm_debug("Node Id: %016" PRIx64 " Value:%s\n",
                   msg->header.source_node_id, buffer);
        } while (0);
        bm_free(buffer);
      }
      break;
    }
    case BYTES: {
      size_t buffer_len = MAX_CONFIG_BUFFER_SIZE_BYTES;
      uint8_t *buffer = (uint8_t *)bm_malloc(buffer_len);
      if (buffer) {
        do {
          if (cbor_value_copy_byte_string(&it, buffer, &buffer_len, NULL) !=
              CborNoError) {
            break;
          }
          bm_debug("Node Id: %016" PRIx64 " Value: ",
                   msg->header.source_node_id);
          for (size_t i = 0; i < buffer_len; i++) {
            bm_debug("0x%02x:", buffer[i]);
            if (i % 8 == 0) {
              bm_debug("\n");
            }
          }
          bm_debug("\n");
        } while (0);
        bm_free(buffer);
      }
      break;
    }
    case ARRAY: {
      bm_debug("Node Id: %016" PRIx64 " Value: Array\n",
               msg->header.source_node_id);
      size_t buffer_len = MAX_CONFIG_BUFFER_SIZE_BYTES;
      uint8_t *buffer = (uint8_t *)bm_malloc(buffer_len);
      if (buffer) {
        for (size_t i = 0; i < buffer_len; i++) {
          bm_debug(" %02x", buffer[i]);
          if (i % 16 == 15) {
            bm_debug("\n");
          }
        }
        bm_debug("\n");
        bm_free(buffer);
      }
      break;
    }
    }
  } while (0);
}

static bool bcmp_config_send_del_key_response(uint64_t target_node_id,
                                              BmConfigPartition partition,
                                              size_t key_len, const char *key,
                                              bool success, uint16_t seq_num) {
  bool rval = false;
  if (key) {
    size_t msg_size = sizeof(BmConfigDeleteKeyResponse) + key_len;
    BmConfigDeleteKeyResponse *del_resp =
        (BmConfigDeleteKeyResponse *)bm_malloc(msg_size);
    if (del_resp) {
      del_resp->header.target_node_id = target_node_id;
      del_resp->header.source_node_id = node_id();
      del_resp->partition = partition;
      del_resp->key_length = key_len;
      memcpy(del_resp->key, key, key_len);
      del_resp->success = success;
      if (bcmp_tx(&multicast_ll_addr, BcmpConfigDeleteResponseMessage,
                  (uint8_t *)del_resp, msg_size, seq_num, NULL) == BmOK) {
        rval = true;
      }
      bm_free(del_resp);
    }
  }
  return rval;
}

static void bcmp_process_del_request_message(BmConfigDeleteKeyRequest *msg,
                                             uint16_t seq_num) {
  if (msg) {
    do {
      bool success = remove_key(msg->partition, msg->key, msg->key_length);
      if (!bcmp_config_send_del_key_response(msg->header.source_node_id,
                                             msg->partition, msg->key_length,
                                             msg->key, success, seq_num)) {
        bm_debug("Failed to send del key resp\n");
      }
    } while (0);
  }
}

static void bcmp_process_del_response_message(BmConfigDeleteKeyResponse *msg) {
  if (msg) {
    char *key_print_buf = (char *)bm_malloc(msg->key_length + 1);
    if (key_print_buf) {
      memcpy(key_print_buf, msg->key, msg->key_length);
      key_print_buf[msg->key_length] = '\0';
      bm_debug("Node Id: %016" PRIx64
               " Key Delete Response - Key: %s, Partition: %d, Success %d\n",
               msg->header.source_node_id, key_print_buf, msg->partition,
               msg->success);
      bm_free(key_print_buf);
    }
  }
}

/*!
    \return true if the caller should forward the message, false if the message was handled
*/
static BmErr bcmp_process_config_message(BcmpProcessData data) {
  BmErr err = BmEINVAL;
  bool should_forward = false;
  BmConfigHeader *msg_header = (BmConfigHeader *)data.payload;

  if (msg_header->target_node_id != node_id()) {
    should_forward = true;
  } else {
    err = BmOK;
    switch (data.header->type) {
    case BcmpConfigGetMessage: {
      bcmp_config_process_config_get_msg((BmConfigGet *)(data.payload),
                                         data.header->seq_num);
      break;
    }
    case BcmpConfigSetMessage: {
      bcmp_config_process_config_set_msg((BmConfigSet *)data.payload,
                                         data.header->seq_num);
      break;
    }
    case BcmpConfigCommitMessage: {
      bcmp_config_process_commit_msg((BmConfigCommit *)data.payload);

      break;
    }
    case BcmpConfigStatusRequestMessage: {
      bcmp_config_process_status_request_msg(
          (BmConfigStatusRequest *)data.payload, data.header->seq_num);
      break;
    }
    case BcmpConfigStatusResponseMessage: {
      BmConfigStatusResponse *msg = (BmConfigStatusResponse *)data.payload;
      bm_debug("Response msg -- Node Id: %016" PRIx64
               ", Partition: %d, Commit Status: %d\n",
               msg->header.source_node_id, msg->partition, msg->committed);
      bm_debug("Num Keys: %d\n", msg->num_keys);
      BmConfigStatusKeyData *key = (BmConfigStatusKeyData *)msg->keyData;
      for (int i = 0; i < msg->num_keys; i++) {
        char *key_buf = (char *)bm_malloc(key->key_length + 1);
        if (key_buf) {
          memcpy(key_buf, key->key, key->key_length);
          key_buf[key->key_length] = '\0';
          bm_debug("%s\n", key_buf);
          key += key->key_length + sizeof(BmConfigStatusKeyData);
          bm_free(key_buf);
        }
      }
      break;
    }
    case BcmpConfigValueMessage: {
      BmConfigValue *msg = (BmConfigValue *)data.payload;
      bcmp_process_value_message(msg);
      break;
    }
    case BcmpConfigDeleteRequestMessage: {
      BmConfigDeleteKeyRequest *msg = (BmConfigDeleteKeyRequest *)data.payload;
      bcmp_process_del_request_message(msg, data.header->seq_num);
      break;
    }
    case BcmpConfigDeleteResponseMessage: {
      BmConfigDeleteKeyResponse *msg =
          (BmConfigDeleteKeyResponse *)data.payload;
      bcmp_process_del_response_message(msg);
      break;
    }
    default:
      bm_debug("Invalid config msg\n");
      break;
    }
  }

  if (should_forward) {
    err = bcmp_ll_forward(data.header, data.payload, data.size,
                          data.ingress_port);
  }

  return err;
}

BmErr bcmp_config_init(void) {
  BmErr err = BmOK;
  BcmpPacketCfg config_get = {
      false,
      true,
      bcmp_process_config_message,
  };
  BcmpPacketCfg config_set = {
      false,
      true,
      bcmp_process_config_message,
  };
  BcmpPacketCfg config_commit = {
      false,
      false,
      bcmp_process_config_message,
  };
  BcmpPacketCfg config_status_request = {
      false,
      true,
      bcmp_process_config_message,
  };
  BcmpPacketCfg config_status_response = {
      true,
      false,
      bcmp_process_config_message,
  };
  BcmpPacketCfg config_delete_request = {
      false,
      true,
      bcmp_process_config_message,
  };
  BcmpPacketCfg config_delete_response = {
      true,
      false,
      bcmp_process_config_message,
  };
  BcmpPacketCfg config_value = {
      true,
      false,
      bcmp_process_config_message,
  };

  bm_err_check(err, packet_add(&config_get, BcmpConfigGetMessage));
  bm_err_check(err, packet_add(&config_set, BcmpConfigSetMessage));
  bm_err_check(err, packet_add(&config_commit, BcmpConfigCommitMessage));
  bm_err_check(
      err, packet_add(&config_status_request, BcmpConfigStatusRequestMessage));
  bm_err_check(err, packet_add(&config_status_response,
                               BcmpConfigStatusResponseMessage));
  bm_err_check(
      err, packet_add(&config_delete_request, BcmpConfigDeleteRequestMessage));
  bm_err_check(err, packet_add(&config_delete_response,
                               BcmpConfigDeleteResponseMessage));
  bm_err_check(err, packet_add(&config_value, BcmpConfigValueMessage));
  return err;
}
