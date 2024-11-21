#include "bcmp.h"
#include "fff.h"
#include "messages.h"
#include "util.h"

typedef BmErr (*BcmpReplyCb)(uint8_t *);

DECLARE_FAKE_VALUE_FUNC(BmErr, bcmp_init, NetworkDevice);
DECLARE_FAKE_VALUE_FUNC(BmErr, bcmp_tx, const void *, BcmpMessageType,
                        uint8_t *, uint16_t, uint32_t, BcmpReplyCb)
DECLARE_FAKE_VALUE_FUNC(BmErr, bcmp_ll_forward, BcmpHeader *, void *, uint32_t,
                        uint8_t);
DECLARE_FAKE_VOID_FUNC(bcmp_link_change, uint8_t, bool);
