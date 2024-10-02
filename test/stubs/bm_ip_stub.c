#include "mock_bm_ip.h"
#include "util.h"

DEFINE_FFF_GLOBALS;

DEFINE_FAKE_VALUE_FUNC(BmErr, bm_ip_init, BmQueue);
DEFINE_FAKE_VOID_FUNC(bm_ip_rx_cleanup, void *);
DEFINE_FAKE_VALUE_FUNC(void *, bm_ip_tx_new, const void *, uint32_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_ip_tx_copy, void *, const void *, uint32_t,
                       uint32_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_ip_tx_perform, void *, const void *);
DEFINE_FAKE_VOID_FUNC(bm_ip_tx_cleanup, void *);
