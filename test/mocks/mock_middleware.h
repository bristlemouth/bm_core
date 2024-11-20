#include "fff.h"
#include "util.h"

DECLARE_FAKE_VALUE_FUNC(BmErr, bm_middleware_local_pub, void *, uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_middleware_init, uint16_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_middleware_net_tx, void *, uint32_t);
