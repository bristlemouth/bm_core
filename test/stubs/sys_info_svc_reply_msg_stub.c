#include "mock_sys_info_svc_reply_msg.h"

DEFINE_FAKE_VALUE_FUNC(CborError, sys_info_reply_encode, SysInfoReplyData *,
                       uint8_t *, size_t, size_t *);
DEFINE_FAKE_VALUE_FUNC(CborError, sys_info_reply_decode, SysInfoReplyData *,
                       const uint8_t *, size_t);
