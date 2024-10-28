#include "mock_config_cbor_map_srv_request_msg.h"

DEFINE_FAKE_VALUE_FUNC(CborError, config_cbor_map_request_encode,
                       ConfigCborMapRequestData *, uint8_t *, size_t, size_t *);

DEFINE_FAKE_VALUE_FUNC(CborError, config_cbor_map_request_decode,
                       ConfigCborMapRequestData *, const uint8_t *, size_t);
