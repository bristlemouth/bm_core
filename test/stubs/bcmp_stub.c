#include "mock_bcmp.h"

DEFINE_FAKE_VALUE_FUNC(BmErr, bcmp_init, NetworkDevice);
DEFINE_FAKE_VALUE_FUNC(BmErr, bcmp_tx, const BmIpAddr *, BcmpMessageType,
                       uint8_t *, uint16_t, uint32_t, BcmpReplyCb)
DEFINE_FAKE_VALUE_FUNC(BmErr, bcmp_ll_forward, BcmpHeader *, void *, uint32_t,
                       uint8_t);
DEFINE_FAKE_VOID_FUNC(bcmp_link_change, uint8_t, bool);
