#include <gtest/gtest.h>
#include <helpers.hpp>

#include "fff.h"

DEFINE_FFF_GLOBALS;

extern "C" {
#include "cbor.h"
#include "cbor_service_helper.h"
#include "mock_bm_os.h"
}

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

class CborServiceHelper : public ::testing::Test {
protected:
  CborServiceHelper() {}
  ~CborServiceHelper() override {}
  void setUp(void) { config_init(); }
};

TEST_F(CborServiceHelper, cbor_map) {
  CborEncoder encoder;
  uint8_t cbor_buf[MAX_STR_LEN_BYTES];

  uint32_t foo = 42;
  memset(cbor_buf, 0, MAX_STR_LEN_BYTES);
  cbor_encoder_init(&encoder, cbor_buf, MAX_STR_LEN_BYTES, 0);
  cbor_encode_uint(&encoder, foo);
  set_config_cbor(BM_CFG_PARTITION_SYSTEM, "foo", strlen("foo"), cbor_buf,
                  MAX_STR_LEN_BYTES);

  int32_t bar = -1000;
  memset(cbor_buf, 0, MAX_STR_LEN_BYTES);
  cbor_encoder_init(&encoder, cbor_buf, MAX_STR_LEN_BYTES, 0);
  cbor_encode_int(&encoder, bar);
  set_config_cbor(BM_CFG_PARTITION_SYSTEM, "bar", strlen("bar"), cbor_buf,
                  MAX_STR_LEN_BYTES);

  float baz = 3.14159;
  memset(cbor_buf, 0, MAX_STR_LEN_BYTES);
  cbor_encoder_init(&encoder, cbor_buf, MAX_STR_LEN_BYTES, 0);
  cbor_encode_float(&encoder, baz);
  set_config_cbor(BM_CFG_PARTITION_SYSTEM, "baz", strlen("baz"), cbor_buf,
                  MAX_STR_LEN_BYTES);

  const char *silly = "The quick brown fox jumps over the lazy dog";
  memset(cbor_buf, 0, MAX_STR_LEN_BYTES);
  cbor_encoder_init(&encoder, cbor_buf, MAX_STR_LEN_BYTES, 0);
  cbor_encode_text_stringz(&encoder, silly);
  set_config_cbor(BM_CFG_PARTITION_SYSTEM, "silly", strlen("silly"), cbor_buf,
                  MAX_STR_LEN_BYTES);

  uint8_t bytes[] = {0xde, 0xad, 0xbe, 0xef, 0x5a,
                     0xad, 0xda, 0xad, 0xb0, 0xdd};
  memset(cbor_buf, 0, MAX_STR_LEN_BYTES);
  cbor_encoder_init(&encoder, cbor_buf, MAX_STR_LEN_BYTES, 0);
  cbor_encode_byte_string(&encoder, bytes, array_size(bytes));
  set_config_cbor(BM_CFG_PARTITION_SYSTEM, "bytes", strlen("bytes"), cbor_buf,
                  MAX_STR_LEN_BYTES);

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
