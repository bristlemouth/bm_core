#include "configuration.h"
#include "bm_config.h"
#include "bm_configs_generic.h"
#include "crc.h"
#include <inttypes.h>
#include <stdio.h>
#ifndef CBOR_CUSTOM_ALLOC_INCLUDE
#error "CBOR_CUSTOM_ALLOC_INCLUDE must be defined!"configuration.c
#endif // CBOR_CUSTOM_ALLOC_INCLUDE
#ifndef CBOR_PARSER_MAX_RECURSIONS
#error "CBOR_PARSER_MAX_RECURSIONS must be defined!"
#endif // CBOR_PARSER_MAX_RECURSIONS
#define RAM_CONFIG_BUFFER_SIZE (10 * 1024)

struct ConfigInfo {
  uint8_t ram_buffer[RAM_CONFIG_BUFFER_SIZE];
  bool needs_commit;
};

static struct ConfigInfo CONFIGS[BM_CFG_PARTITION_COUNT];

static bool find_key_idx(BmConfigPartition partition, const char *key,
                         size_t len, uint8_t *idx) {
  ConfigPartition *config_partition =
      (ConfigPartition *)CONFIGS[partition].ram_buffer;
  bool ret = false;

  for (int i = 0; i < config_partition->header.numKeys; i++) {
    if (strncmp(key, config_partition->keys[i].key_buf, len) == 0) {
      *idx = i;
      ret = true;
      break;
    }
  }

  return ret;
}

/*!
 @brief Determines if a key is valid or not

 @details Key must have alphabetical or numerical numbers, key can also have
          underscores and terminating characters

 @param key Key to evaluate
 @param key_len Length of key to evaluat

 @return true if key is valid, false if key is not valid
 */
static inline bool is_key_valid(const char *key, uint32_t key_len) {
  bool ret = true;

  for (uint32_t i = 0; i < key_len; i++) {
    if (!(key[i] >= '0' && key[i] <= '9') &&
        !(key[i] >= 'a' && key[i] <= 'z') &&
        !(key[i] >= 'A' && key[i] <= 'Z') && key[i] != '_' && key[i] != '\0') {
      bm_debug("Improper character in key at index: %" PRIu32 "\n",
               (uint32_t)i);
      ret = false;
      break;
    }
  }

  return ret;
}

static bool prepare_cbor_encoder(BmConfigPartition partition, const char *key,
                                 size_t key_len, CborEncoder *encoder,
                                 uint8_t *key_idx, bool *key_exists) {
  bool ret = false;
  ConfigPartition *config_partition =
      (ConfigPartition *)CONFIGS[partition].ram_buffer;

  if (key) {
    do {
      if (key_len > MAX_KEY_LEN_BYTES) {
        break;
      }
      if (config_partition->header.numKeys >= MAX_NUM_KV) {
        break;
      }
      *key_exists = find_key_idx(partition, key, key_len, key_idx);
      if (!*key_exists) {
        if (!is_key_valid(key, key_len)) {
          break;
        }
        *key_idx = config_partition->header.numKeys;
      }
      if (snprintf(config_partition->keys[*key_idx].key_buf,
                   sizeof(config_partition->keys[*key_idx].key_buf), "%s",
                   key) < 0) {
        break;
      }
      cbor_encoder_init(encoder, config_partition->values[*key_idx].valueBuffer,
                        sizeof(config_partition->values[*key_idx].valueBuffer),
                        0);
      ret = true;
    } while (0);
  }

  return ret;
}

static bool prepare_cbor_parser(BmConfigPartition partition, const char *key,
                                size_t key_len, CborValue *it,
                                CborParser *parser) {
  bool ret = false;
  uint8_t key_idx;
  ConfigPartition *config_partition =
      (ConfigPartition *)CONFIGS[partition].ram_buffer;

  if (key) {
    do {
      if (key_len > MAX_KEY_LEN_BYTES) {
        break;
      }
      if (!find_key_idx(partition, key, key_len, &key_idx)) {
        break;
      }
      if (cbor_parser_init(
              config_partition->values[key_idx].valueBuffer,
              sizeof(config_partition->values[key_idx].valueBuffer), 0, parser,
              it) != CborNoError) {
        break;
      }
      ret = true;
    } while (0);
  }

  return ret;
}

static bool load_and_verify_nvm_config(ConfigPartition *config_partition,
                                       BmConfigPartition partition) {
  bool ret = false;

  do {
    if (!bm_config_read(partition, CONFIG_START_OFFSET_IN_BYTES,
                        (uint8_t *)config_partition, sizeof(ConfigPartition),
                        CONFIG_LOAD_TIMEOUT_MS)) {
      break;
    }
    uint32_t partition_crc32 = crc32_ieee(
        (const uint8_t *)&config_partition->header.version,
        (sizeof(ConfigPartition) - sizeof(config_partition->header.crc32)));
    if (config_partition->header.crc32 != partition_crc32) {
      break;
    }
    ret = true;
  } while (0);
  return ret;
}

void config_init(void) {
  for (uint8_t i = 0; i < BM_CFG_PARTITION_COUNT; i++) {
    ConfigPartition *config_partition =
        (ConfigPartition *)CONFIGS[i].ram_buffer;
    if (!load_and_verify_nvm_config(config_partition, (BmConfigPartition)i)) {
      bm_debug("Unable to load configs from flash.");
      config_partition->header.numKeys = 0;
      config_partition->header.version = CONFIG_VERSION;
      // TODO: Once we have default configs, load these into flash.
    } else {
      bm_debug("Succesfully loaded configs from flash.");
    }
  }
}

/*!
 @brief Get uint32 config

 @param key[in] - null terminated key
 @param value[out] - value

 @returns - true if success, false otherwise.
*/
bool get_config_uint(BmConfigPartition partition, const char *key,
                     size_t key_len, uint32_t *value) {
  bool ret = false;

  CborValue it;
  CborParser parser;

  if (key) {
    do {
      if (!prepare_cbor_parser(partition, key, key_len, &it, &parser)) {
        break;
      }
      uint64_t temp;
      if (!cbor_value_is_unsigned_integer(&it)) {
        break;
      }
      if (cbor_value_get_uint64(&it, &temp) != CborNoError) {
        break;
      }
      *value = (uint32_t)temp;
      ret = true;
    } while (0);
  }
  return ret;
}

/*!
 @brief Get int32 config

 @param key[in] - null terminated key
 @param value[out] - value

 @returns - true if success, false otherwise.
*/
bool get_config_int(BmConfigPartition partition, const char *key,
                    size_t key_len, int32_t *value) {
  bool ret = false;
  CborValue it;
  CborParser parser;

  if (key) {
    do {
      if (!prepare_cbor_parser(partition, key, key_len, &it, &parser)) {
        break;
      }
      int64_t temp;
      if (!cbor_value_is_integer(&it)) {
        break;
      }
      if (cbor_value_get_int64(&it, &temp) != CborNoError) {
        break;
      }
      *value = (int32_t)temp;
      ret = true;
    } while (0);
  }

  return ret;
}

/*!
 @brief Get float config

 @param key[in] - null terminated key
 @param value[out] - value

 @returns - true if success, false otherwise.
*/
bool get_config_float(BmConfigPartition partition, const char *key,
                      size_t key_len, float *value) {
  bool ret = false;
  CborValue it;
  CborParser parser;

  if (key) {
    do {
      if (!prepare_cbor_parser(partition, key, key_len, &it, &parser)) {
        break;
      }
      float temp;
      if (!cbor_value_is_float(&it)) {
        break;
      }
      if (cbor_value_get_float(&it, &temp) != CborNoError) {
        break;
      }
      *value = temp;
      ret = true;
    } while (0);
  }

  return ret;
}

/*!
* Get str config
* \param key[in] - null terminated key
* \param value[out] - value (non null terminated)
* \param value_len[in/out] - in: buffer size, out:bytes len
* \returns - true if success, false otherwise.
*/
bool get_config_string(BmConfigPartition partition, const char *key,
                       size_t key_len, char *value, size_t *value_len) {
  bool ret = false;

  CborValue it;
  CborParser parser;

  if (key && value) {
    do {
      if (!prepare_cbor_parser(partition, key, key_len, &it, &parser)) {
        break;
      }
      if (!cbor_value_is_text_string(&it)) {
        break;
      }
      if (cbor_value_copy_text_string(&it, value, value_len, NULL) !=
          CborNoError) {
        break;
      }
      ret = true;
    } while (0);
  }

  return ret;
}

/*!
* Get bytes config
* \param key[in] - null terminated key
* \param value[out] - value
* \param value_len[in/out] - in: buffer size, out:bytes len
* \returns - true if success, false otherwise.
*/
bool get_config_buffer(BmConfigPartition partition, const char *key,
                       size_t key_len, uint8_t *value, size_t *value_len) {
  bool ret = false;

  CborValue it;
  CborParser parser;

  if (key && value) {
    do {
      if (!prepare_cbor_parser(partition, key, key_len, &it, &parser)) {
        break;
      }
      if (!cbor_value_is_byte_string(&it)) {
        break;
      }
      if (cbor_value_copy_byte_string(&it, value, value_len, NULL) !=
          CborNoError) {
        break;
      }
      ret = true;
    } while (0);
  }

  return ret;
}

/*!
* Get the cbor encoded buffer for a given key.
* \param key[in] - null terminated key
* \param value[out] - value buffer
* \param value_len[in/out] -  in: buffer size, out :buffer len
* \returns - true if success, false otherwise.
*/
bool get_config_cbor(BmConfigPartition partition, const char *key,
                     size_t key_len, uint8_t *value, size_t *value_len) {
  bool ret = false;
  ConfigPartition *config_partition =
      (ConfigPartition *)CONFIGS[partition].ram_buffer;
  CborValue it;
  CborParser parser;
  uint8_t key_idx;

  if (key && value) {
    do {
      if (!prepare_cbor_parser(partition, key, key_len, &it, &parser)) {
        break;
      }
      if (!cbor_value_is_valid(&it)) {
        break;
      }
      if (!find_key_idx(partition, key, key_len, &key_idx)) {
        break;
      }
      size_t buffer_size =
          sizeof(config_partition->values[key_idx].valueBuffer);
      if (*value_len < buffer_size || value_len == 0) {
        break;
      }
      memcpy(value, config_partition->values[key_idx].valueBuffer, buffer_size);
      *value_len = buffer_size;
      ret = true;
    } while (0);
  }

  return ret;
}

/*!
* sets uint32 config
* \param key[in] - null terminated key
* \param value[in] - value
* \returns - true if success, false otherwise.
*/
bool set_config_uint(BmConfigPartition partition, const char *key,
                     size_t key_len, uint32_t value) {
  bool ret = false;
  ConfigPartition *config_partition =
      (ConfigPartition *)CONFIGS[partition].ram_buffer;
  CborEncoder encoder;
  uint8_t key_idx;
  bool key_exists;

  if (key) {
    do {
      if (!prepare_cbor_encoder(partition, key, key_len, &encoder, &key_idx,
                                &key_exists)) {
        break;
      }
      if (cbor_encode_uint(&encoder, value) != CborNoError) {
        break;
      }
      config_partition->keys[key_idx].value_type = UINT32;
      config_partition->keys[key_idx].key_len = key_len;
      if (!key_exists) {
        config_partition->header.numKeys++;
      }
      CONFIGS[partition].needs_commit = true;
      ret = true;
    } while (0);
  }

  return ret;
}

/*!
* sets int32 config
* \param key[in] -  null terminated key
* \param value[in] - value
* \returns - true if success, false otherwise.
*/
bool set_config_int(BmConfigPartition partition, const char *key,
                    size_t key_len, int32_t value) {
  bool ret = false;
  ConfigPartition *config_partition =
      (ConfigPartition *)CONFIGS[partition].ram_buffer;
  CborEncoder encoder;
  uint8_t key_idx;
  bool key_exists;

  if (key) {
    do {
      if (!prepare_cbor_encoder(partition, key, key_len, &encoder, &key_idx,
                                &key_exists)) {
        break;
      }
      if (cbor_encode_int(&encoder, value) != CborNoError) {
        break;
      }
      config_partition->keys[key_idx].value_type = INT32;
      config_partition->keys[key_idx].key_len = key_len;
      if (!key_exists) {
        config_partition->header.numKeys++;
      }
      CONFIGS[partition].needs_commit = true;
      ret = true;
    } while (0);
  }

  return ret;
}

/*!
* sets float config
* \param key[in] -  null terminated key
* \param value[in] - value
* \returns - true if success, false otherwise.
*/
bool set_config_float(BmConfigPartition partition, const char *key,
                      size_t key_len, float value) {
  bool ret = false;
  ConfigPartition *config_partition =
      (ConfigPartition *)CONFIGS[partition].ram_buffer;
  CborEncoder encoder;
  uint8_t key_idx;
  bool key_exists;

  if (key) {
    do {
      if (!prepare_cbor_encoder(partition, key, key_len, &encoder, &key_idx,
                                &key_exists)) {
        break;
      }
      if (cbor_encode_float(&encoder, value) != CborNoError) {
        break;
      }
      config_partition->keys[key_idx].value_type = FLOAT;
      config_partition->keys[key_idx].key_len = key_len;
      if (!key_exists) {
        config_partition->header.numKeys++;
      }
      CONFIGS[partition].needs_commit = true;
      ret = true;
    } while (0);
  }

  return ret;
}

/*!
* sets a null-terminated str config
* \param key[in] -  null terminated key
* \param value[in] - value
* \returns - true if success, false otherwise.
*/
bool set_config_string(BmConfigPartition partition, const char *key,
                       size_t key_len, const char *value, size_t value_len) {
  bool ret = false;
  ConfigPartition *config_partition =
      (ConfigPartition *)CONFIGS[partition].ram_buffer;
  CborEncoder encoder;
  uint8_t key_idx;
  bool key_exists;

  if (key) {
    do {
      if (!prepare_cbor_encoder(partition, key, key_len, &encoder, &key_idx,
                                &key_exists)) {
        break;
      }
      if (cbor_encode_text_string(&encoder, value, value_len) != CborNoError) {
        break;
      }
      config_partition->keys[key_idx].value_type = STR;
      config_partition->keys[key_idx].key_len = key_len;
      if (!key_exists) {
        config_partition->header.numKeys++;
      }
      CONFIGS[partition].needs_commit = true;
      ret = true;
    } while (0);
  }

  return ret;
}

/*!
* sets bytes config
* \param key[in] -  null terminated key
* \param value[in] - value
* \param value_len[in] - bytes len
* \returns - true if success, false otherwise.
*/
bool set_config_buffer(BmConfigPartition partition, const char *key,
                       size_t key_len, const uint8_t *value, size_t value_len) {
  bool ret = false;
  ConfigPartition *config_partition =
      (ConfigPartition *)CONFIGS[partition].ram_buffer;
  CborEncoder encoder;
  uint8_t key_idx;
  bool key_exists;

  if (key) {
    do {
      if (!prepare_cbor_encoder(partition, key, key_len, &encoder, &key_idx,
                                &key_exists)) {
        break;
      }
      if (cbor_encode_byte_string(&encoder, value, value_len) != CborNoError) {
        break;
      }
      config_partition->keys[key_idx].value_type = BYTES;
      config_partition->keys[key_idx].key_len = key_len;
      if (!key_exists) {
        config_partition->header.numKeys++;
      }
      CONFIGS[partition].needs_commit = true;
      ret = true;
    } while (0);
  }

  return ret;
}

/*!
* Sets any cbor conifguration to a given key,
* \param key[in] -  null terminated key
* \param value[in] - value buffer
* \param value_len[in] - buffer len
* \returns - true if success, false otherwise.
*/
bool set_config_cbor(BmConfigPartition partition, const char *key,
                     size_t key_len, uint8_t *value, size_t value_len) {
  CborValue it;
  CborParser parser;
  uint8_t key_idx;
  bool ret = false;
  ConfigPartition *config_partition =
      (ConfigPartition *)CONFIGS[partition].ram_buffer;

  if (key && value) {
    do {
      if (key_len > MAX_KEY_LEN_BYTES) {
        break;
      }
      if (value_len > MAX_CONFIG_BUFFER_SIZE_BYTES || value_len == 0) {
        break;
      }
      if (cbor_parser_init(value, value_len, 0, &parser, &it) != CborNoError) {
        break;
      }
      if (!cbor_value_is_valid(&it)) {
        break;
      }
      if (!find_key_idx(partition, key, key_len, &key_idx)) {
        if (!is_key_valid(key, key_len)) {
          break;
        }
        if (config_partition->header.numKeys >= MAX_NUM_KV) {
          break;
        }
        key_idx = config_partition->header.numKeys;
        if (snprintf(config_partition->keys[key_idx].key_buf,
                     sizeof(config_partition->keys[key_idx].key_buf), "%s",
                     key) < 0) {
          break;
        }
      }
      ConfigDataTypes type;
      if (!cbor_type_to_config(&it, &type)) {
        break;
      }
      config_partition->keys[key_idx].value_type = type;
      config_partition->keys[key_idx].key_len = key_len;
      memcpy(config_partition->values[key_idx].valueBuffer, value, value_len);
      if (key_idx == config_partition->header.numKeys) {
        config_partition->header.numKeys++;
      }
      CONFIGS[partition].needs_commit = true;
      ret = true;
    } while (0);
  }

  return ret;
}

/*!
* Sets any cbor conifguration to a given key,
* \param cbor[in] -  cbor type to convert from
* \param config_type[out] - config type to convert to
* \returns - true if success, false otherwise.
*/
bool cbor_type_to_config(const CborValue *value, ConfigDataTypes *config_type) {
  bool ret = true;
  do {
    if (cbor_value_is_integer(value)) {
      if (cbor_value_is_unsigned_integer(value)) {
        *config_type = UINT32;
        break;
      } else {
        *config_type = INT32;
        break;
      }
    } else if (cbor_value_is_byte_string(value)) {
      *config_type = BYTES;
      break;
    } else if (cbor_value_is_text_string(value)) {
      *config_type = STR;
      break;
    } else if (cbor_value_is_float(value)) {
      *config_type = FLOAT;
      break;
    } else if (cbor_value_is_array(value)) {
      *config_type = ARRAY;
      break;
    }
    ret = false;
  } while (0);
  return ret;
}

/*!
* Gets a list of the keys stored in the configuration.
* \param num_stored_key[out] - number of keys stored
* \returns - map of keys
*/
const ConfigKey *get_stored_keys(BmConfigPartition partition,
                                 uint8_t *num_stored_keys) {
  ConfigPartition *config_partition =
      (ConfigPartition *)CONFIGS[partition].ram_buffer;
  *num_stored_keys = config_partition->header.numKeys;
  return config_partition->keys;
}

/*!
* Remove a key/value from the store.
* \param key[in] - null terminated key
* \returns - true if successful, false otherwise.
*/
bool remove_key(BmConfigPartition partition, const char *key, size_t key_len) {
  bool ret = false;
  ConfigPartition *config_partition =
      (ConfigPartition *)CONFIGS[partition].ram_buffer;
  uint8_t key_idx;

  if (key) {
    do {
      if (!find_key_idx(partition, key, key_len, &key_idx)) {
        break;
      }
      if (config_partition->header.numKeys - 1 >
          key_idx) { // if there are keys after, we need to move them up.
        memmove(&config_partition->keys[key_idx],
                &config_partition->keys[key_idx + 1],
                (config_partition->header.numKeys - 1 - key_idx) *
                    sizeof(ConfigKey)); // shift keys
        memmove(&config_partition->values[key_idx],
                &config_partition->values[key_idx + 1],
                (config_partition->header.numKeys - 1 - key_idx) *
                    sizeof(ConfigValue)); // shift values
      }
      config_partition->header.numKeys--;
      CONFIGS[partition].needs_commit = true;
      ret = true;
    } while (0);
  }

  return ret;
}

const char *data_type_enum_to_str(ConfigDataTypes type) {
  const char *ret = NULL;

  switch (type) {
  case UINT32:
    ret = "uint32";
    break;
  case INT32:
    ret = "int32";
    break;
  case FLOAT:
    ret = "float";
    break;
  case STR:
    ret = "str";
    break;
  case BYTES:
    ret = "bytes";
    break;
  case ARRAY:
    ret = "array";
    break;
  }

  return ret;
}

/*!
 @brief Clear a partition

 @details Erases all keys and values in a partition.

 @param partition Partition to erase

 @return true if partition was successfully erased, false otherwise
 */
bool clear_partition(BmConfigPartition partition) {
  bool ret = false;

  if (partition < BM_CFG_PARTITION_COUNT) {
    ConfigPartition *config_partition =
        (ConfigPartition *)CONFIGS[partition].ram_buffer;
    ret = true;
    memset(config_partition, 0, sizeof(CONFIGS[partition].ram_buffer));

    config_partition->header.numKeys = 0;
    config_partition->header.version = CONFIG_VERSION;
    CONFIGS[partition].needs_commit = true;
  }

  return ret;
}

bool save_config(BmConfigPartition partition, bool restart) {
  bool ret = false;
  ConfigPartition *config_partition =
      (ConfigPartition *)CONFIGS[partition].ram_buffer;

  do {
    config_partition->header.crc32 = crc32_ieee(
        (const uint8_t *)&config_partition->header.version,
        (sizeof(ConfigPartition) - sizeof(config_partition->header.crc32)));
    if (!bm_config_write(partition, CONFIG_START_OFFSET_IN_BYTES,
                         (uint8_t *)config_partition, sizeof(ConfigPartition),
                         CONFIG_LOAD_TIMEOUT_MS)) {
      break;
    }
    if (restart) {
      bm_config_reset();
    }
    CONFIGS[partition].needs_commit = false;
    ret = true;
  } while (0);

  return ret;
}

bool get_value_size(BmConfigPartition partition, const char *key,
                    size_t key_len, size_t *size) {
  bool ret = false;
  CborValue it;
  CborParser parser;

  if (key) {
    do {
      if (!prepare_cbor_parser(partition, key, key_len, &it, &parser)) {
        break;
      }
      ConfigDataTypes config_type;
      if (!cbor_type_to_config(&it, &config_type)) {
        break;
      }
      switch (config_type) {
      case UINT32: {
        *size = sizeof(uint32_t);
        break;
      }
      case INT32: {
        *size = sizeof(int32_t);
        break;
      }
      case FLOAT: {
        *size = sizeof(float);
        break;
      }
      case STR:
      case BYTES: {
        if (cbor_value_get_string_length(&it, size) != CborNoError) {
          return false;
        }
        break;
      }
      case ARRAY: {
        if (cbor_value_get_array_length(&it, size) != CborNoError) {
          return false;
        }
        break;
      }
      }
      ret = true;
    } while (0);
  }

  return ret;
}

bool needs_commit(BmConfigPartition partition) {
  bool ret = false;

  if (partition < BM_CFG_PARTITION_COUNT) {
    ret = CONFIGS[partition].needs_commit;
  }

  return ret;
}
