#include "mock_configuration.h"

DEFINE_FAKE_VOID_FUNC(config_init);
DEFINE_FAKE_VALUE_FUNC(bool, get_config_uint, BmConfigPartition, const char *,
                       size_t, uint32_t *);
DEFINE_FAKE_VALUE_FUNC(bool, get_config_int, BmConfigPartition, const char *,
                       size_t, int32_t *);
DEFINE_FAKE_VALUE_FUNC(bool, get_config_float, BmConfigPartition, const char *,
                       size_t, float *);
DEFINE_FAKE_VALUE_FUNC(bool, get_config_string, BmConfigPartition, const char *,
                       size_t, char *, size_t *);
DEFINE_FAKE_VALUE_FUNC(bool, get_config_buffer, BmConfigPartition, const char *,
                       size_t, uint8_t *, size_t *);
DEFINE_FAKE_VALUE_FUNC(bool, get_config_cbor, BmConfigPartition, const char *,
                       size_t, uint8_t *, size_t *);
DEFINE_FAKE_VALUE_FUNC(bool, set_config_uint, BmConfigPartition, const char *,
                       size_t, uint32_t);
DEFINE_FAKE_VALUE_FUNC(bool, set_config_int, BmConfigPartition, const char *,
                       size_t, int32_t);
DEFINE_FAKE_VALUE_FUNC(bool, set_config_float, BmConfigPartition, const char *,
                       size_t, float);
DEFINE_FAKE_VALUE_FUNC(bool, set_config_string, BmConfigPartition, const char *,
                       size_t, const char *, size_t);
DEFINE_FAKE_VALUE_FUNC(bool, set_config_buffer, BmConfigPartition, const char *,
                       size_t, const uint8_t *, size_t);
DEFINE_FAKE_VALUE_FUNC(bool, set_config_cbor, BmConfigPartition, const char *,
                       size_t, uint8_t *, size_t);
DEFINE_FAKE_VALUE_FUNC(const ConfigKey *, get_stored_keys, BmConfigPartition,
                       uint8_t *);
DEFINE_FAKE_VALUE_FUNC(bool, remove_key, BmConfigPartition, const char *,
                       size_t);
DEFINE_FAKE_VALUE_FUNC(const char *, data_type_enum_to_str, ConfigDataTypes);
DEFINE_FAKE_VALUE_FUNC(bool, save_config, BmConfigPartition, bool);
DEFINE_FAKE_VALUE_FUNC(bool, get_value_size, BmConfigPartition, const char *,
                       size_t, size_t *);
DEFINE_FAKE_VALUE_FUNC(bool, needs_commit, BmConfigPartition);
DEFINE_FAKE_VALUE_FUNC(bool, cbor_type_to_config, const CborValue *,
                       ConfigDataTypes *);
