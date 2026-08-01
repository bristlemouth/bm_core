#include "mock_hash.h"

DEFINE_FAKE_VALUE_FUNC(Hash *, hash_create, uint16_t, uint16_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, hash_delete, Hash *);
DEFINE_FAKE_VALUE_FUNC(BmErr, hash_create_static, Hash *, void *, uint32_t *,
                       uint16_t, uint16_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, hash_insert, Hash *, uint32_t, const void *);
DEFINE_FAKE_VALUE_FUNC(BmErr, hash_look_up, Hash *, uint32_t, void *);
DEFINE_FAKE_VALUE_FUNC(BmErr, hash_remove, Hash *, uint32_t);
DEFINE_FAKE_VALUE_FUNC(uint16_t, hash_get_count, Hash *);
DEFINE_FAKE_VALUE_FUNC(uint8_t, hash_get_load, Hash *);
