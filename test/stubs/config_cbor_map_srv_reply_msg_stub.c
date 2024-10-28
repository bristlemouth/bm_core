#include "mock_config_cbor_map_srv_reply_msg.h"

DEFINE_FAKE_VALUE_FUNC(CborError, config_cbor_map_reply_encode,
                       ConfigCborMapReplyData *, uint8_t *, size_t, size_t *);

DEFINE_FAKE_VALUE_FUNC(CborError, config_cbor_map_reply_decode,
                       ConfigCborMapReplyData *, const uint8_t *, size_t);
