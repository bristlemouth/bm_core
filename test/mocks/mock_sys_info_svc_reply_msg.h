#include "fff.h"
#include "sys_info_svc_reply_msg.h"

DECLARE_FAKE_VALUE_FUNC(CborError, sys_info_reply_encode, SysInfoReplyData *,
                        uint8_t *, size_t, size_t *);
DECLARE_FAKE_VALUE_FUNC(CborError, sys_info_reply_decode, SysInfoReplyData *,
                        const uint8_t *, size_t);
