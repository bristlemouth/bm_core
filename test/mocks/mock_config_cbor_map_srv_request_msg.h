#include "config_cbor_map_srv_request_msg.h"
#include "fff.h"

DECLARE_FAKE_VALUE_FUNC(CborError, config_cbor_map_request_encode,
                        ConfigCborMapRequestData *, uint8_t *, size_t,
                        size_t *);
DECLARE_FAKE_VALUE_FUNC(CborError, config_cbor_map_request_decode,
                        ConfigCborMapRequestData *, const uint8_t *, size_t);
