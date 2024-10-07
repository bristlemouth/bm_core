#ifndef __BM_CONFIGS_GENERIC_H__
#define __BM_CONFIGS_GENERIC_H__


#include "util.h"
#include "messages.h"
#include <stdint.h>

#define BM_MAX_KEY_LEN_BYTES 32
#define BM_MAX_NUM_KV 50
#define BM_MAX_KEY_LEN_BYTES 32
#define BM_MAX_STR_LEN_BYTES 50
#define BM_MAX_CONFIG_BUFFER_SIZE_BYTES 50
#define BM_CONFIG_VERSION 0

typedef enum {
  UINT32,
  INT32,
  FLOAT,
  STR,
  BYTES,
  ARRAY,
} GenericConfigDataTypes;

typedef struct {
  char keyBuffer[BM_MAX_KEY_LEN_BYTES];
  size_t keyLen;
  GenericConfigDataTypes valueType;
} __attribute__((packed, aligned(1))) GenericConfigKey;

const GenericConfigKey *bcmp_config_get_stored_keys(uint8_t &num_stored_keys, BmConfigPartition partition);
bool bcmp_remove_key(const char *key, size_t key_len, BmConfigPartition partition);
bool bcmp_config_needs_commit(BmConfigPartition partition);
bool bcmp_commit_config(BmConfigPartition partition);
bool bcmp_set_config(const char *key, size_t key_len, uint8_t *value,
                     size_t value_len, BmConfigPartition partition);
bool bcmp_get_config(const char *key, size_t key_len, uint8_t *value,
                     size_t &value_len, BmConfigPartition partition);
bool bm_cbor_type_to_config_type(const CborValue *value, ConfigDataTypes &configType);

#endif // __BM_CONFIGS_GENERIC_H__