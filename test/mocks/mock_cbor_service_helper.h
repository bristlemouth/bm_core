#include "cbor_service_helper.h"
#include "fff.h"

DECLARE_FAKE_VALUE_FUNC(uint8_t *, services_cbor_as_map, size_t *,
                        BmConfigPartition);
DECLARE_FAKE_VALUE_FUNC(uint32_t, services_cbor_encoded_as_crc32,
                        BmConfigPartition);
