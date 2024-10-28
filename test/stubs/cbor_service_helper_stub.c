#include "mock_cbor_service_helper.h"

DEFINE_FAKE_VALUE_FUNC(uint8_t *, services_cbor_as_map, size_t *,
                       BmConfigPartition);
DEFINE_FAKE_VALUE_FUNC(uint32_t, services_cbor_encoded_as_crc32,
                       BmConfigPartition);
