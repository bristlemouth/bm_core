#include "gtest/gtest.h"

#include "fff.h"

extern "C" {
#include "bm_configs_generic.h"
#include "configuration.h"
}

DEFINE_FFF_GLOBALS;

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

class ConfigurationTest : public ::testing::Test {
protected:
  ConfigurationTest() {}

  ~ConfigurationTest() override {}

  void SetUp() override {}
  void TearDown() override {}
  // Objects declared here can be used by all tests in the test suite for Foo.
};

TEST_F(ConfigurationTest, BasicTest) {
  config_init();

  uint8_t num_keys;
  const ConfigKey *key_list = NULL;
  key_list = get_stored_keys(BM_CFG_PARTITION_SYSTEM, &num_keys);
  EXPECT_EQ(num_keys, 0);
  EXPECT_EQ((key_list != NULL), true);

  //   uint32_t
  uint32_t foo = 42;
  uint32_t result_foo = 0;
  EXPECT_EQ(set_config_uint(BM_CFG_PARTITION_SYSTEM, "foo", strlen("foo"), foo),
            true);
  EXPECT_EQ(get_config_uint(BM_CFG_PARTITION_SYSTEM, "foo", strlen("foo"),
                            &result_foo),
            true);
  EXPECT_EQ(foo, result_foo);
  key_list = get_stored_keys(BM_CFG_PARTITION_SYSTEM, &num_keys);
  EXPECT_EQ(num_keys, 1);
  EXPECT_EQ(strncmp("foo", key_list[0].key_buf, sizeof("foo")), 0);

  // Second set. (overwrite)
  foo = 999;
  EXPECT_EQ(set_config_uint(BM_CFG_PARTITION_SYSTEM, "foo", strlen("foo"), foo),
            true);
  EXPECT_EQ(get_config_uint(BM_CFG_PARTITION_SYSTEM, "foo", strlen("foo"),
                            &result_foo),
            true);
  EXPECT_EQ(foo, result_foo);
  key_list = get_stored_keys(BM_CFG_PARTITION_SYSTEM, &num_keys);
  EXPECT_EQ(num_keys, 1);
  EXPECT_EQ(strncmp("foo", key_list[0].key_buf, sizeof("foo")), 0);

  // int32_t
  int32_t bar = -1000;
  int32_t result_bar = 0;
  EXPECT_EQ(set_config_int(BM_CFG_PARTITION_SYSTEM, "bar", strlen("bar"), bar),
            true);
  EXPECT_EQ(get_config_int(BM_CFG_PARTITION_SYSTEM, "bar", strlen("bar"),
                           &result_bar),
            true);
  EXPECT_EQ(get_config_uint(BM_CFG_PARTITION_SYSTEM, "foo", strlen("foo"),
                            &result_foo),
            true);
  EXPECT_EQ(foo, result_foo);
  EXPECT_EQ(bar, result_bar);
  key_list = get_stored_keys(BM_CFG_PARTITION_SYSTEM, &num_keys);
  EXPECT_EQ(num_keys, 2);
  EXPECT_EQ(strncmp("bar", key_list[1].key_buf, sizeof("bar")), 0);
  EXPECT_EQ(strncmp("foo", key_list[0].key_buf, sizeof("foo")), 0);

  //  float
  float baz = 3.14159;
  float result_baz = 0;
  EXPECT_EQ(
      set_config_float(BM_CFG_PARTITION_SYSTEM, "baz", strlen("baz"), baz),
      true);
  EXPECT_EQ(get_config_float(BM_CFG_PARTITION_SYSTEM, "baz", strlen("baz"),
                             &result_baz),
            true);
  EXPECT_EQ(baz, result_baz);
  key_list = get_stored_keys(BM_CFG_PARTITION_SYSTEM, &num_keys);
  EXPECT_EQ(num_keys, 3);
  EXPECT_EQ(strncmp("baz", key_list[2].key_buf, sizeof("baz")), 0);
  EXPECT_EQ(strncmp("bar", key_list[1].key_buf, sizeof("bar")), 0);
  EXPECT_EQ(strncmp("foo", key_list[0].key_buf, sizeof("foo")), 0);

  // str
  const char *silly = "The quick brown fox jumps over the lazy dog";
  char result_silly[100];
  size_t size = sizeof(result_silly);
  EXPECT_EQ(set_config_string(BM_CFG_PARTITION_SYSTEM, "silly", strlen("silly"),
                              silly, strlen(silly)),
            true);
  EXPECT_EQ(get_config_string(BM_CFG_PARTITION_SYSTEM, "silly", strlen("silly"),
                              result_silly, &size),
            true);
  EXPECT_EQ(size, strlen(silly));
  EXPECT_EQ(strncmp(silly, result_silly, strlen(silly)), 0);
  key_list = get_stored_keys(BM_CFG_PARTITION_SYSTEM, &num_keys);
  EXPECT_EQ(num_keys, 4);
  EXPECT_EQ(strncmp("silly", key_list[3].key_buf, sizeof("silly")), 0);
  EXPECT_EQ(strncmp("baz", key_list[2].key_buf, sizeof("baz")), 0);
  EXPECT_EQ(strncmp("bar", key_list[1].key_buf, sizeof("bar")), 0);
  EXPECT_EQ(strncmp("foo", key_list[0].key_buf, sizeof("foo")), 0);

  // Bytes
  uint8_t bytes[] = {0xde, 0xad, 0xbe, 0xef, 0x5a,
                     0xad, 0xda, 0xad, 0xb0, 0xdd};
  uint8_t result_bytes[10];
  size = sizeof(result_bytes);
  EXPECT_EQ(set_config_buffer(BM_CFG_PARTITION_SYSTEM, "bytes", strlen("bytes"),
                              bytes, sizeof(bytes)),
            true);
  EXPECT_EQ(get_config_buffer(BM_CFG_PARTITION_SYSTEM, "bytes", strlen("bytes"),
                              result_bytes, &size),
            true);
  EXPECT_EQ(size, sizeof(bytes));
  EXPECT_EQ(memcmp(bytes, result_bytes, size), 0);
  key_list = get_stored_keys(BM_CFG_PARTITION_SYSTEM, &num_keys);
  EXPECT_EQ(num_keys, 5);
  EXPECT_EQ(strncmp("bytes", key_list[4].key_buf, sizeof("bytes")), 0);
  EXPECT_EQ(strncmp("silly", key_list[3].key_buf, sizeof("silly")), 0);
  EXPECT_EQ(strncmp("baz", key_list[2].key_buf, sizeof("baz")), 0);
  EXPECT_EQ(strncmp("bar", key_list[1].key_buf, sizeof("bar")), 0);
  EXPECT_EQ(strncmp("foo", key_list[0].key_buf, sizeof("foo")), 0);

  // Remove key
  EXPECT_EQ(remove_key(BM_CFG_PARTITION_SYSTEM, "foo", strlen("foo")), true);
  EXPECT_EQ(get_config_uint(BM_CFG_PARTITION_SYSTEM, "foo", strlen("foo"),
                            &result_foo),
            false);
  key_list = get_stored_keys(BM_CFG_PARTITION_SYSTEM, &num_keys);
  EXPECT_EQ(num_keys, 4);
  EXPECT_EQ(strncmp("bytes", key_list[3].key_buf, sizeof("bytes")), 0);
  EXPECT_EQ(strncmp("silly", key_list[2].key_buf, sizeof("silly")), 0);
  EXPECT_EQ(strncmp("baz", key_list[1].key_buf, sizeof("baz")), 0);
  EXPECT_EQ(strncmp("bar", key_list[0].key_buf, sizeof("bar")), 0);
  EXPECT_EQ(get_config_int(BM_CFG_PARTITION_SYSTEM, "bar", strlen("bar"),
                           &result_bar),
            true);
  EXPECT_EQ(bar, result_bar);
  EXPECT_EQ(get_config_float(BM_CFG_PARTITION_SYSTEM, "baz", strlen("baz"),
                             &result_baz),
            true);
  EXPECT_EQ(baz, result_baz);
  size = sizeof(result_silly);
  EXPECT_EQ(get_config_string(BM_CFG_PARTITION_SYSTEM, "silly", strlen("silly"),
                              result_silly, &size),
            true);
  EXPECT_EQ(size, strlen(silly));
  EXPECT_EQ(strncmp(silly, result_silly, strlen(silly)), 0);
  size = sizeof(result_bytes);
  EXPECT_EQ(get_config_buffer(BM_CFG_PARTITION_SYSTEM, "bytes", strlen("bytes"),
                              result_bytes, &size),
            true);
  EXPECT_EQ(size, sizeof(bytes));
  EXPECT_EQ(memcmp(bytes, result_bytes, size), 0);
}

TEST_F(ConfigurationTest, NoKeyFound) {
  config_init();
  uint32_t result_foo = 0;
  EXPECT_EQ(get_config_uint(BM_CFG_PARTITION_SYSTEM, "foo", strlen("foo"),
                            &result_foo),
            false);
  uint32_t foo = 42;
  EXPECT_EQ(set_config_uint(BM_CFG_PARTITION_SYSTEM, "foo", strlen("foo"), foo),
            true);
  EXPECT_EQ(get_config_uint(BM_CFG_PARTITION_SYSTEM, "baz", strlen("baz"),
                            &result_foo),
            false);
}

TEST_F(ConfigurationTest, TooMuchThingy) {
  config_init();

  const char *silly = "The quick brown fox jumps over the lazy dog The quick "
                      "brown fox jumps over the lazy dog";
  EXPECT_EQ(set_config_string(BM_CFG_PARTITION_SYSTEM, "silly", strlen("silly"),
                              silly, strlen(silly)),
            false);
  uint8_t bytes[100];
  memset(bytes, 0xa5, sizeof(bytes));
  EXPECT_EQ(set_config_buffer(BM_CFG_PARTITION_SYSTEM, "bytes", strlen("bytes"),
                              bytes, sizeof(bytes)),
            false);
}

//TEST_F(ConfigurationTest, TooLittleStorage)
//{
//  const ext_flash_partition_t test_configuration = {
//      .fa_off = 4096,
//      .fa_size = 10000,
//  };
//  uint8_t small_storage;
//  NvmPartition testPartition(_storage, test_configuration);
//  EXPECT_DEATH(Configuration config(testPartition,&small_storage,sizeof(small_storage)), "");
//}

TEST_F(ConfigurationTest, NoKeyToRemove) {
  config_init();
  EXPECT_EQ(remove_key(BM_CFG_PARTITION_SYSTEM, "foo", strlen("foo")), false);
}

TEST_F(ConfigurationTest, cborGetSet) {
  uint8_t cborBuffer[MAX_STR_LEN_BYTES];
  config_init();
  uint8_t num_keys;
  const ConfigKey *key_list = NULL;
  key_list = get_stored_keys(BM_CFG_PARTITION_SYSTEM, &num_keys);
  EXPECT_EQ(num_keys, 0);
  EXPECT_EQ((key_list != NULL), true);

  uint32_t foo = 42;
  uint32_t result_foo = 0;
  EXPECT_EQ(set_config_uint(BM_CFG_PARTITION_SYSTEM, "foo", strlen("foo"), foo),
            true);
  EXPECT_EQ(get_config_uint(BM_CFG_PARTITION_SYSTEM, "foo", strlen("foo"),
                            &result_foo),
            true);
  EXPECT_EQ(foo, result_foo);
  key_list = get_stored_keys(BM_CFG_PARTITION_SYSTEM, &num_keys);
  EXPECT_EQ(num_keys, 1);
  EXPECT_EQ(strncmp("foo", key_list[0].key_buf, sizeof("foo")), 0);
  size_t buffer_size = sizeof(cborBuffer);
  EXPECT_EQ(get_config_cbor(BM_CFG_PARTITION_SYSTEM, "foo", strlen("foo"),
                            cborBuffer, &buffer_size),
            true);
  EXPECT_EQ(set_config_cbor(BM_CFG_PARTITION_SYSTEM, "bar", strlen("foo"),
                            cborBuffer, buffer_size),
            true);
  key_list = get_stored_keys(BM_CFG_PARTITION_SYSTEM, &num_keys);
  EXPECT_EQ(num_keys, 2);
  uint32_t result_bar;
  EXPECT_EQ(get_config_uint(BM_CFG_PARTITION_SYSTEM, "bar", strlen("bar"),
                            &result_bar),
            true);
  EXPECT_EQ(foo, result_bar);
  size_t bar_size;
  EXPECT_EQ(
      get_value_size(BM_CFG_PARTITION_SYSTEM, "bar", strlen("bar"), &bar_size),
      true);
  EXPECT_EQ(bar_size, sizeof(foo));

  buffer_size = sizeof(cborBuffer);
  const char *silly = "The quick brown fox jumps over the lazy dog";
  EXPECT_EQ(set_config_string(BM_CFG_PARTITION_SYSTEM, "silly", strlen("silly"),
                              silly, strlen(silly)),
            true);
  EXPECT_EQ(get_config_cbor(BM_CFG_PARTITION_SYSTEM, "silly", strlen("silly"),
                            cborBuffer, &buffer_size),
            true);
  EXPECT_EQ(set_config_cbor(BM_CFG_PARTITION_SYSTEM, "bar", strlen("bar"),
                            cborBuffer, buffer_size),
            true);
  key_list = get_stored_keys(BM_CFG_PARTITION_SYSTEM, &num_keys);
  EXPECT_EQ(num_keys, 3);
  char silly_bar[100];
  size_t size = sizeof(silly_bar);
  EXPECT_EQ(get_config_string(BM_CFG_PARTITION_SYSTEM, "bar", strlen("bar"),
                              silly_bar, &size),
            true);
  EXPECT_EQ(strncmp(silly, silly_bar, strlen(silly)), 0);
  EXPECT_EQ(
      get_value_size(BM_CFG_PARTITION_SYSTEM, "bar", strlen("bar"), &bar_size),
      true);
  EXPECT_EQ(bar_size, strlen(silly));
}

TEST_F(ConfigurationTest, BadCborGetSet) {
  uint8_t cborBuffer[MAX_STR_LEN_BYTES];
  size_t buffer_size = sizeof(cborBuffer);
  memset(cborBuffer, 0xFF, buffer_size);
  config_init();
  uint8_t num_keys;
  const ConfigKey *key_list = NULL;
  key_list = get_stored_keys(BM_CFG_PARTITION_SYSTEM, &num_keys);
  EXPECT_EQ(num_keys, 0);
  EXPECT_EQ((key_list != NULL), true);
  EXPECT_EQ(get_config_cbor(BM_CFG_PARTITION_SYSTEM, "foo", strlen("foo"),
                            cborBuffer, &buffer_size),
            false); // Key doesn't exist.
  EXPECT_EQ(set_config_cbor(BM_CFG_PARTITION_SYSTEM, "foo", strlen("foo"),
                            cborBuffer, buffer_size),
            false); // Invalid cbor buffer.
  uint32_t foo = 42;
  EXPECT_EQ(set_config_uint(BM_CFG_PARTITION_SYSTEM, "foo", strlen("foo"), foo),
            true);
  EXPECT_EQ(get_config_cbor(BM_CFG_PARTITION_SYSTEM, "foo", strlen("foo"),
                            cborBuffer, &buffer_size),
            true);
  EXPECT_EQ(
      set_config_cbor(BM_CFG_PARTITION_SYSTEM,
                      "a super long key string that shouldn't work",
                      strlen("a super long key string that shouldn't work"),
                      cborBuffer, buffer_size),
      false); // key string too long.
  key_list = get_stored_keys(BM_CFG_PARTITION_SYSTEM, &num_keys);
  EXPECT_EQ(num_keys, 1);
}
