#include "cbor_service_helper.h"
#include "bm_config.h"
#include "bm_os.h"
#include "configuration.h"
#include "crc.h"
#include "messages/config.h"
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

uint8_t *services_cbor_as_map(size_t *buffer_size, BmConfigPartition type) {
  if (!buffer_size) {
    return NULL;
  }
  *buffer_size = 0;
  size_t allocated_size = 512;
  uint8_t *buffer = (uint8_t *)bm_malloc(allocated_size);
  if (!buffer) {
    return NULL;
  }
  uint32_t tmpU;
  int32_t tmpI;
  float tmpF;
  char tmpS[MAX_STR_LEN_BYTES];
  size_t tmpSSize = MAX_STR_LEN_BYTES;
  uint8_t tmpB[MAX_STR_LEN_BYTES];
  size_t tmpBSize = MAX_STR_LEN_BYTES;
  CborEncoder encoder;
  CborEncoder map;
  CborError err;
  CborValue it;
  CborParser parser;
  bool shouldRetry;
  uint8_t num_keys;
  const ConfigKey *keys = get_stored_keys(type, &num_keys);

  do {
    shouldRetry = false;
    cbor_encoder_init(&encoder, buffer, allocated_size, 0);
    err = cbor_encoder_create_map(&encoder, &map, num_keys);
    if (err != CborNoError && err != CborErrorOutOfMemory) {
      break;
    }

    for (uint8_t i = 0; i < num_keys; i++) {
      ConfigKey key = keys[i];
      err = cbor_encode_text_string(&map, key.key_buf, key.key_len);
      if (err != CborNoError && err != CborErrorOutOfMemory) {
        break;
      }

      bool internalSuccess = true;
      tmpBSize = MAX_STR_LEN_BYTES;
      memset(tmpB, 0, MAX_STR_LEN_BYTES);
      if (get_config_cbor(type, key.key_buf, key.key_len, tmpB, &tmpBSize) &&
          cbor_parser_init(tmpB, tmpBSize, 0, &parser, &it) != CborNoError) {
        break;
      }
      if (!cbor_value_is_valid(&it)) {
        break;
      }

      switch (key.value_type) {
      case UINT32: {
        uint64_t temp;
        internalSuccess = (cbor_value_get_uint64(&it, &temp) == CborNoError);
        if (internalSuccess) {
          tmpU = (uint32_t)temp;
          err = cbor_encode_uint(&map, tmpU);
        }
      } break;
      case INT32: {
        int64_t temp;
        internalSuccess = (cbor_value_get_int64(&it, &temp) == CborNoError);
        if (internalSuccess) {
          tmpI = (int32_t)temp;
          err = cbor_encode_int(&map, tmpI);
        }
      } break;
      case FLOAT: {
        internalSuccess = (cbor_value_get_float(&it, &tmpF) == CborNoError);
        if (internalSuccess) {
          err = cbor_encode_float(&map, tmpF);
        }
      } break;
      case STR: {
        tmpSSize = MAX_STR_LEN_BYTES;
        memset(tmpS, 0, MAX_STR_LEN_BYTES);
        do {
          if (cbor_value_copy_text_string(&it, tmpS, &tmpSSize, NULL) !=
              CborNoError) {
            break;
          }
          if (tmpSSize >= MAX_CONFIG_BUFFER_SIZE_BYTES) {
            break;
          }
          tmpS[tmpSSize] = '\0';
          err = cbor_encode_text_string(&map, tmpS, tmpSSize);
        } while (0);
      } break;
      case BYTES: {
        tmpSSize = MAX_STR_LEN_BYTES;
        memset(tmpS, 0, MAX_STR_LEN_BYTES);
        do {
          if (cbor_value_copy_byte_string(&it, (uint8_t *)tmpS, &tmpSSize,
                                          NULL) != CborNoError) {
            break;
          }
          if (tmpSSize >= MAX_CONFIG_BUFFER_SIZE_BYTES) {
            break;
          }
          err = cbor_encode_byte_string(&map, (uint8_t *)tmpS, tmpSSize);
        } while (0);
      } break;
      case ARRAY: {
        if (internalSuccess && map.data.ptr + tmpBSize < map.end) {
          memcpy(map.data.ptr, tmpB, tmpBSize);
          map.data.ptr += tmpBSize;
        }
        break;
      }
      }

      if (!internalSuccess ||
          (err != CborNoError && err != CborErrorOutOfMemory)) {
        break; // out of for loop
      }
    }

    if (err == CborNoError || err == CborErrorOutOfMemory) {
      err = cbor_encoder_close_container(&encoder, &map);
    }

    if (err == CborErrorOutOfMemory) {
      size_t needed = cbor_encoder_get_extra_bytes_needed(&encoder);
      bm_debug(
          "Encoding failed, buffer too small. Will retry. "
          "Need %zu more bytes than the %zu originally given, or %zu total.\n",
          needed, allocated_size, allocated_size + needed);
      bm_free(buffer);
      allocated_size += needed;
      buffer = (uint8_t *)bm_malloc(allocated_size);
      if (!buffer) {
        break;
      }
      shouldRetry = true;
    } else if (err == CborNoError) {
      *buffer_size = cbor_encoder_get_buffer_size(&encoder, buffer);
    } else {
      bm_debug("Failed to encode config as cbor map, err=%" PRIu32 "\n", err);
      bm_free(buffer);
      buffer = NULL;
    }
  } while (shouldRetry);

  return buffer;
}

uint32_t services_cbor_encoded_as_crc32(BmConfigPartition type) {
  uint32_t crc32 = 0;
  size_t buffer_size = 0;
  uint8_t *buffer = services_cbor_as_map(&buffer_size, type);
  if (buffer) {
    crc32 = crc32_ieee(buffer, buffer_size);
    bm_free(buffer);
  }
  return crc32;
}
