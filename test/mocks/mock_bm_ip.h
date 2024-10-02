#include "bm_os.h"
#include "fff.h"
#include "util.h"

DECLARE_FAKE_VALUE_FUNC(BmErr, bm_ip_init, BmQueue);
DECLARE_FAKE_VOID_FUNC(bm_ip_rx_cleanup, void *);
DECLARE_FAKE_VALUE_FUNC(void *, bm_ip_tx_new, const void *, uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_ip_tx_copy, void *, const void *, uint32_t,
                        uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_ip_tx_perform, void *, const void *);
DECLARE_FAKE_VOID_FUNC(bm_ip_tx_cleanup, void *);
