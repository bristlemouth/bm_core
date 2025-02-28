#include "mock_power_info_reply_msg.h"

DEFINE_FAKE_VALUE_FUNC(CborError, power_info_reply_encode, PowerInfoReplyData *,
                       uint8_t *, size_t, size_t *);
DEFINE_FAKE_VALUE_FUNC(CborError, power_info_reply_decode, PowerInfoReplyData *,
                       const uint8_t *, size_t);
