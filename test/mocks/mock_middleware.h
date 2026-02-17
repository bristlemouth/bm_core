#include "fff.h"
#include "middleware.h"
#include "util.h"

DECLARE_FAKE_VALUE_FUNC(BmErr, bm_middleware_rx, uint16_t, void *, uint64_t,
                        uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_middleware_init);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_middleware_net_tx, uint16_t, void *,
                        uint32_t);
void bm_middleware_invoke_cb(uint16_t port, uint64_t node_id, void *buf,
                             uint32_t size);
