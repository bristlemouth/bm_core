#include "fff.h"
#include "l2.h"
#include "util.h"

DECLARE_FAKE_VALUE_FUNC(BmErr, bm_l2_link_output, void *, uint32_t);
DECLARE_FAKE_VOID_FUNC(bm_l2_deinit)
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_l2_init, NetworkDevice);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_l2_register_link_change_callback,
                        L2LinkChangeCb);
DECLARE_FAKE_VALUE_FUNC(bool, bm_l2_get_port_state, uint8_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_l2_netif_set_power, bool);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_l2_netif_enable_disable_port, uint8_t, bool);
