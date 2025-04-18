#include "config_cbor_map_srv_reply_msg.h"
#include "fff.h"

DECLARE_FAKE_VALUE_FUNC(CborError, config_cbor_map_reply_encode,
                        ConfigCborMapReplyData *, uint8_t *, size_t, size_t *);

DECLARE_FAKE_VALUE_FUNC(CborError, config_cbor_map_reply_decode,
                        ConfigCborMapReplyData *, const uint8_t *, size_t);
