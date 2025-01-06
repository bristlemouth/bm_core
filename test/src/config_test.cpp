#include <gtest/gtest.h>
#include <helpers.hpp>

#include "fff.h"

DEFINE_FFF_GLOBALS;

extern "C" {
#include "bm_os.h"
#include "cbor.h"
#include "messages/config.h"
}

#define ENCODE_BUFFER_SIZE 512
#define DECODE_BUFFER_SIZE (ENCODE_BUFFER_SIZE / 2)

bool bm_config_read(BmConfigPartition partition, uint32_t offset,
                    uint8_t *buffer, size_t length, uint32_t timeout_ms) {
  bool ret = false;

  switch (partition) {
  case BM_CFG_PARTITION_USER:
  case BM_CFG_PARTITION_SYSTEM:
  case BM_CFG_PARTITION_HARDWARE:
    ret = true;
    break;
  default:
    break;
  }

  return ret;
}
bool bm_config_write(BmConfigPartition partition, uint32_t offset,
                     uint8_t *buffer, size_t length, uint32_t timeout_ms) {
  bool ret = false;

  switch (partition) {
  case BM_CFG_PARTITION_USER:
  case BM_CFG_PARTITION_SYSTEM:
  case BM_CFG_PARTITION_HARDWARE:
    ret = true;
    break;
  default:
    break;
  }

  return ret;
}
void bm_config_reset(void) {}

class Config : public ::testing::Test {
public:
  rnd_gen RND;

private:
protected:
  Config() {}
  ~Config() override {}

  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(Config, decode) {

  uint8_t cbor_buf[ENCODE_BUFFER_SIZE] = {0};
  CborEncoder encoder = {};
  struct {
    uint8_t fail;
    uint32_t get;
    uint32_t set;
  } u32;
  struct {
    int8_t fail;
    int32_t get;
    int32_t set;
  } s32;
  struct {
    uint8_t fail;
    float get;
    float set;
  } f;
  struct {
    char *large_get;
    char *get;
    char *set;
  } str;
  struct {
    uint8_t *large_get;
    uint8_t *get;
    uint8_t *set;
  } bytes;

  size_t size = 0;

  // Test uint32
  u32.fail = 0;
  u32.get = 0;
  u32.set = RND.rnd_int(UINT32_MAX, 0);
  cbor_encoder_init(&encoder, cbor_buf, sizeof(cbor_buf), 0);
  cbor_encode_uint(&encoder, u32.set);

  size = sizeof(u32.get);
  EXPECT_EQ(bcmp_config_decode_value(UINT32, cbor_buf, sizeof(cbor_buf),
                                     (void *)&u32.get, &size),
            BmOK);
  EXPECT_EQ(u32.get, u32.set);
  EXPECT_EQ(size, sizeof(u32.set));

  size = sizeof(u32.fail);
  EXPECT_NE(bcmp_config_decode_value(UINT32, cbor_buf, sizeof(cbor_buf),
                                     (void *)&u32.fail, &size),
            BmOK);
  EXPECT_EQ(u32.fail, 0);
  EXPECT_NE(size, sizeof(u32.set));

  // Test int32
  s32.fail = 0;
  s32.get = 0;
  s32.set = RND.rnd_int(INT32_MAX, INT32_MIN);
  cbor_encoder_init(&encoder, cbor_buf, sizeof(cbor_buf), 0);
  cbor_encode_int(&encoder, s32.set);

  size = sizeof(s32.get);
  EXPECT_EQ(bcmp_config_decode_value(INT32, cbor_buf, sizeof(cbor_buf),
                                     (void *)&s32.get, &size),
            BmOK);
  EXPECT_EQ(s32.get, s32.set);
  EXPECT_EQ(size, sizeof(s32.set));

  size = sizeof(s32.fail);
  EXPECT_NE(bcmp_config_decode_value(INT32, cbor_buf, sizeof(cbor_buf),
                                     (void *)&s32.fail, &size),
            BmOK);
  EXPECT_EQ(s32.fail, 0);
  EXPECT_NE(size, sizeof(s32.set));

  // Test float
  f.fail = 0;
  f.get = 0;
  f.set = 1234.56789;
  cbor_encoder_init(&encoder, cbor_buf, sizeof(cbor_buf), 0);
  cbor_encode_float(&encoder, f.set);

  size = sizeof(f.get);
  EXPECT_EQ(bcmp_config_decode_value(FLOAT, cbor_buf, sizeof(cbor_buf),
                                     (void *)&f.get, &size),
            BmOK);
  EXPECT_EQ(f.get, f.set);
  EXPECT_EQ(size, sizeof(f.set));

  size = sizeof(f.fail);
  EXPECT_NE(bcmp_config_decode_value(FLOAT, cbor_buf, sizeof(cbor_buf),
                                     (void *)&f.fail, &size),
            BmOK);
  EXPECT_EQ(f.fail, 0);
  EXPECT_NE(size, sizeof(f.set));

  // Test string
  str.large_get = (char *)bm_malloc(ENCODE_BUFFER_SIZE);
  str.get = (char *)bm_malloc(DECODE_BUFFER_SIZE);
  str.set = (char *)bm_malloc(DECODE_BUFFER_SIZE);
  memset(str.get, 0, DECODE_BUFFER_SIZE);
  memset(str.large_get, 0, ENCODE_BUFFER_SIZE);
  RND.rnd_str(str.set, DECODE_BUFFER_SIZE);
  cbor_encoder_init(&encoder, cbor_buf, sizeof(cbor_buf), 0);
  cbor_encode_text_string(&encoder, str.set, DECODE_BUFFER_SIZE);

  size = DECODE_BUFFER_SIZE;
  EXPECT_EQ(bcmp_config_decode_value(STR, cbor_buf, sizeof(cbor_buf),
                                     (void *)str.get, &size),
            BmOK);
  ASSERT_EQ(size, DECODE_BUFFER_SIZE);
  EXPECT_EQ(memcmp(str.get, str.set, size), 0);

  size = ENCODE_BUFFER_SIZE;
  EXPECT_EQ(bcmp_config_decode_value(STR, cbor_buf, sizeof(cbor_buf),
                                     (void *)str.large_get, &size),
            BmOK);
  ASSERT_EQ(size, DECODE_BUFFER_SIZE);
  EXPECT_EQ(memcmp(str.large_get, str.set, size), 0);
  bm_free(str.get);
  bm_free(str.set);
  bm_free(str.large_get);

  // Test byte buffer
  bytes.large_get = (uint8_t *)bm_malloc(ENCODE_BUFFER_SIZE);
  bytes.get = (uint8_t *)bm_malloc(DECODE_BUFFER_SIZE);
  bytes.set = (uint8_t *)bm_malloc(DECODE_BUFFER_SIZE);
  memset(bytes.get, 0, DECODE_BUFFER_SIZE);
  memset(bytes.large_get, 0, ENCODE_BUFFER_SIZE);
  RND.rnd_array(bytes.set, DECODE_BUFFER_SIZE);
  cbor_encoder_init(&encoder, cbor_buf, sizeof(cbor_buf), 0);
  cbor_encode_byte_string(&encoder, bytes.set, DECODE_BUFFER_SIZE);

  size = DECODE_BUFFER_SIZE;
  EXPECT_EQ(bcmp_config_decode_value(BYTES, cbor_buf, sizeof(cbor_buf),
                                     (void *)bytes.get, &size),
            BmOK);
  ASSERT_EQ(size, DECODE_BUFFER_SIZE);
  EXPECT_EQ(memcmp(bytes.get, bytes.set, size), 0);

  size = ENCODE_BUFFER_SIZE;
  EXPECT_EQ(bcmp_config_decode_value(BYTES, cbor_buf, sizeof(cbor_buf),
                                     (void *)bytes.large_get, &size),
            BmOK);
  ASSERT_EQ(size, DECODE_BUFFER_SIZE);
  EXPECT_EQ(memcmp(bytes.large_get, bytes.set, size), 0);
  bm_free(bytes.get);
  bm_free(bytes.set);
  bm_free(bytes.large_get);
}
