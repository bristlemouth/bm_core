#include <stdint.h>
#include "util.h"

// config_parition→getStoredKeys()
// config_parition→needsCommit()
// config_partition→getConfigCbor()
// config_partition→removeKey()
// config_partition→saveConfig()
// config_partition→setConfigCbor()

#define BM_MAX_KEY_LEN_BYTES 32

typedef enum ConfigDataTypes{
    UINT32,
    INT32,
    FLOAT,
    STR,
    BYTES,
    ARRAY,
} ConfigDataTypes;

typedef struct ConfigKey {
    char keyBuffer[BM_MAX_KEY_LEN_BYTES];
    size_t keyLen;
    ConfigDataTypes valueType;
} __attribute__((packed, aligned(1))) ConfigKey;

const ConfigKey* bcmp_config_get_stored_keys(uint8_t &num_stored_keys);
bool bcmp_remove_key(const char * key, size_t key_len);
bool bcmp_config_needs_commit(void); // TODO - converge on commit vs save as the naming convention!
bool bcmp_save_config(bool restart=true);
bool bcmp_set_config(const char * key, size_t key_len, uint8_t *value, size_t value_len); // TODO - maybe make these not called ..Cbor... and just have the cbor conversion happen in the user defined function
bool bcmp_get_config(const char * key, size_t key_len, uint8_t *value, size_t &value_len);// that way the user can define their own conversion functions and are not necessarily tied to cbor

