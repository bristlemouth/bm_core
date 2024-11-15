#include "mock_middleware.h"

DEFINE_FAKE_VALUE_FUNC(BmErr, bm_middleware_local_pub, void *, uint32_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_middleware_init, uint16_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_middleware_net_tx, void *, uint32_t);
