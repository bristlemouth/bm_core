#include "mock_l2.h"

DEFINE_FAKE_VALUE_FUNC(BmErr, bm_l2_link_output, void *, uint32_t);
DEFINE_FAKE_VOID_FUNC(bm_l2_deinit)
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_l2_init, NetworkDevice);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_l2_register_link_change_callback,
                       L2LinkChangeCb);
DEFINE_FAKE_VALUE_FUNC(bool, bm_l2_get_port_state, uint8_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_l2_netif_set_power, bool);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_l2_netif_enable_disable_port, uint8_t, bool);
