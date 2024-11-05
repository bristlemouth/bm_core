#include "mock_l2.h"

DEFINE_FAKE_VALUE_FUNC(BmErr, bm_l2_link_output, void *, uint32_t);
DEFINE_FAKE_VOID_FUNC(bm_l2_deinit)
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_l2_init, BmL2ModuleLinkChangeCb,
                       const BmNetDevCfg *, uint8_t);
DEFINE_FAKE_VALUE_FUNC(bool, bm_l2_get_device_handle, uint8_t, void **,
                       uint32_t *);
DEFINE_FAKE_VALUE_FUNC(uint8_t, bm_l2_get_num_ports);
DEFINE_FAKE_VALUE_FUNC(uint8_t, bm_l2_get_num_devices);
DEFINE_FAKE_VALUE_FUNC(bool, bm_l2_get_port_state, uint8_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_l2_netif_set_power, void *, bool);
