#include "util.h"
#include <stdint.h>

#define BM_MAX_KEY_LEN_BYTES 32

typedef enum {
  BM_CFG_PARTITION_USER,
  BM_CFG_PARTITION_SYSTEM,
  BM_CFG_PARTITION_HARDWARE,
} BmConfigPartition;

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
  ConfigDataTypes valueType;
} __attribute__((packed, aligned(1))) GenericConfigKey;

const GenericConfigKey *bcmp_config_get_stored_keys(uint8_t &num_stored_keys, BmConfigPartition partition);
bool bcmp_remove_key(const char *key, size_t key_len, BmConfigPartition partition);
bool bcmp_config_needs_commit(void);
bool bcmp_commit_config(BmConfigPartition partition);
bool bcmp_set_config(const char *key, size_t key_len, uint8_t *value,
                     size_t value_len, BmConfigPartition partition);
bool bcmp_get_config(const char *key, size_t key_len, uint8_t *value,
                     size_t &value_len, BmConfigPartition partition);
