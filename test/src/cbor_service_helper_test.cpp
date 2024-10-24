#include <gtest/gtest.h>
#include <helpers.hpp>

#include "fff.h"

DEFINE_FFF_GLOBALS;

extern "C" {
#include "cbor.h"
#include "cbor_service_helper.h"
#include "mock_bm_os.h"
}

static GenericConfigKey keys[BM_MAX_NUM_KV];
static uint8_t values[BM_MAX_NUM_KV][BM_MAX_STR_LEN_BYTES];
static uint8_t key_idx = 0;

bool find_key_idx(const char *key, size_t len, uint8_t &idx) {
  bool rval = false;
  for (int i = 0; i < key_idx; i++) {
    if (strncmp(key, keys[i].keyBuffer, len) == 0) {
      idx = i;
      rval = true;
      break;
    }
  }
  return rval;
}

bool prepare_cbor_parser(const char *key, size_t key_len, CborValue &it,
                         CborParser &parser) {
  bool retval = false;
  uint8_t idx;
  do {
    if (key_len > BM_MAX_KEY_LEN_BYTES) {
      break;
    }
    if (!find_key_idx(key, key_len, idx)) {
      break;
    }
    if (cbor_parser_init(values[idx], sizeof(values[idx]), 0, &parser, &it) !=
        CborNoError) {
      break;
    }
    retval = true;
  } while (0);
  return retval;
}

bool cbor_type_to_config_type(const CborValue *value,
                              GenericConfigDataTypes &configType) {
  bool rval = true;
  do {
    if (cbor_value_is_integer(value)) {
      if (cbor_value_is_unsigned_integer(value)) {
        configType = UINT32;
        break;
      } else {
        configType = INT32;
        break;
      }
    } else if (cbor_value_is_byte_string(value)) {
      configType = BYTES;
      break;
    } else if (cbor_value_is_text_string(value)) {
      configType = STR;
      break;
    } else if (cbor_value_is_float(value)) {
      configType = FLOAT;
      break;
    } else if (cbor_value_is_array(value)) {
      configType = ARRAY;
      break;
    }
    rval = false;
  } while (0);
  return rval;
}

const GenericConfigKey *
bcmp_config_get_stored_keys(uint8_t *num_keys, BmConfigPartition partition) {
  (void)partition;
  *num_keys = key_idx;
  return (const GenericConfigKey *)keys;
}

bool bcmp_set_config(const char *key, size_t key_len, uint8_t *value,
                     size_t value_len, BmConfigPartition partition) {
  (void)partition;
  CborValue it;
  CborParser parser;
  bool rval = false;
  do {
    if (key_len > BM_MAX_KEY_LEN_BYTES) {
      break;
    }
    if (value_len > BM_MAX_CONFIG_BUFFER_SIZE_BYTES || value_len == 0) {
      break;
    }
    for (size_t i = 0; i < value_len; i++) {
    }
    if (cbor_parser_init(value, value_len, 0, &parser, &it) != CborNoError) {
      break;
    }
    if (!cbor_value_is_valid(&it)) {
      break;
    }
    if (snprintf(keys[key_idx].keyBuffer, sizeof(keys[key_idx].keyBuffer), "%s",
                 key) < 0) {
      break;
    }
    GenericConfigDataTypes type;
    if (!cbor_type_to_config_type(&it, type)) {
      break;
    }
    keys[key_idx].valueType = type;
    keys[key_idx].keyLen = key_len;
    memcpy(values[key_idx], value, value_len);
    key_idx++;
    rval = true;
  } while (0);
  return rval;
}

bool bcmp_get_config(const char *key, size_t key_len, uint8_t *value,
                     size_t *value_len, BmConfigPartition partition) {
  (void)partition;
  bool rval = false;

  CborValue it;
  CborParser parser;
  uint8_t idx;
  do {
    if (!prepare_cbor_parser(key, key_len, it, parser)) {
      break;
    }
    if (!cbor_value_is_valid(&it)) {
      break;
    }
    if (!find_key_idx(key, key_len, idx)) {
      break;
    }
    size_t buffer_size = sizeof(values[idx]);
    if (*value_len < buffer_size || *value_len == 0) {
      break;
    }
    memcpy(value, values[idx], buffer_size);
    *value_len = buffer_size;
    rval = true;
  } while (0);
  return rval;
}

class CborServiceHelper : public ::testing::Test {
protected:
  CborServiceHelper() {}
  ~CborServiceHelper() override {}
};

TEST_F(CborServiceHelper, cbor_map) {
  CborEncoder encoder;
  uint8_t cbor_buf[BM_MAX_STR_LEN_BYTES];

  uint32_t foo = 42;
  memset(cbor_buf, 0, BM_MAX_STR_LEN_BYTES);
  cbor_encoder_init(&encoder, cbor_buf, BM_MAX_STR_LEN_BYTES, 0);
  cbor_encode_uint(&encoder, foo);
  bcmp_set_config("foo", strlen("foo"), cbor_buf, BM_MAX_STR_LEN_BYTES,
                  BM_CFG_PARTITION_SYSTEM);

  int32_t bar = -1000;
  memset(cbor_buf, 0, BM_MAX_STR_LEN_BYTES);
  cbor_encoder_init(&encoder, cbor_buf, BM_MAX_STR_LEN_BYTES, 0);
  cbor_encode_int(&encoder, bar);
  bcmp_set_config("bar", strlen("bar"), cbor_buf, BM_MAX_STR_LEN_BYTES,
                  BM_CFG_PARTITION_SYSTEM);

  float baz = 3.14159;
  memset(cbor_buf, 0, BM_MAX_STR_LEN_BYTES);
  cbor_encoder_init(&encoder, cbor_buf, BM_MAX_STR_LEN_BYTES, 0);
  cbor_encode_float(&encoder, baz);
  bcmp_set_config("baz", strlen("baz"), cbor_buf, BM_MAX_STR_LEN_BYTES,
                  BM_CFG_PARTITION_SYSTEM);

  const char *silly = "The quick brown fox jumps over the lazy dog";
  memset(cbor_buf, 0, BM_MAX_STR_LEN_BYTES);
  cbor_encoder_init(&encoder, cbor_buf, BM_MAX_STR_LEN_BYTES, 0);
  cbor_encode_text_stringz(&encoder, silly);
  bcmp_set_config("silly", strlen("silly"), cbor_buf, BM_MAX_STR_LEN_BYTES,
                  BM_CFG_PARTITION_SYSTEM);

  uint8_t bytes[] = {0xde, 0xad, 0xbe, 0xef, 0x5a,
                     0xad, 0xda, 0xad, 0xb0, 0xdd};
  memset(cbor_buf, 0, BM_MAX_STR_LEN_BYTES);
  cbor_encoder_init(&encoder, cbor_buf, BM_MAX_STR_LEN_BYTES, 0);
  cbor_encode_byte_string(&encoder, bytes, array_size(bytes));
  bcmp_set_config("bytes", strlen("bytes"), cbor_buf, BM_MAX_STR_LEN_BYTES,
                  BM_CFG_PARTITION_SYSTEM);

  size_t buffer_size = 0;
  uint8_t *buffer = services_cbor_as_map(&buffer_size, BM_CFG_PARTITION_SYSTEM);
  EXPECT_NE(buffer, nullptr);

  /*
  From https://cbor.me/
  A5                                    # map(5)
   63                                   # text(3)
      666F6F                            # "foo"
   18 2A                                # unsigned(42)
   63                                   # text(3)
      626172                            # "bar"
   39 03E7                              # negative(999)
   63                                   # text(3)
      62617A                            # "baz"
   FA 40490FD0                          # primitive(1078530000)
   65                                   # text(5)
      73696C6C79                        # "silly"
   78 2B                                # text(43)
      54686520717569636B2062726F776E20666F78206A756D7073206F76657220746865206C617A7920646F67 # "The quick brown fox jumps over the lazy dog"
   65                                   # text(5)
      6279746573                        # "bytes"
   4A                                   # bytes(10)
      DEADBEEF5AADDAADB0DD              # "ޭ\xBE\xEFZ\xADڭ\xB0\xDD"
  */
  EXPECT_EQ(buffer_size, 91);
  EXPECT_EQ(buffer[0], 0xa5);
  EXPECT_EQ(buffer[1], 0x63);
  EXPECT_EQ(buffer[2], 0x66);
  EXPECT_EQ(buffer[3], 0x6f);
  EXPECT_EQ(buffer[4], 0x6f);
  EXPECT_EQ(buffer[5], 0x18);
  EXPECT_EQ(buffer[18], 0xfa);
  EXPECT_EQ(buffer[23], 0x65);
  EXPECT_EQ(buffer[29], 0x78);
  EXPECT_EQ(buffer[30], 0x2b);
  EXPECT_EQ(buffer[74], 0x65);
  EXPECT_EQ(buffer[80], 0x4a);
  bm_free(buffer);
}
