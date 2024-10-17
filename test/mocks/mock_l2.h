#include "fff.h"
#include "l2.h"
#include "util.h"

DECLARE_FAKE_VALUE_FUNC(BmErr, bm_l2_link_output, void *, uint32_t);
DECLARE_FAKE_VOID_FUNC(bm_l2_deinit)
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_l2_init, BmL2ModuleLinkChangeCb,
                        const BmNetDevCfg *, uint8_t);

DECLARE_FAKE_VALUE_FUNC(bool, bm_l2_get_device_handle, uint8_t, void **,
                        uint32_t *);
DECLARE_FAKE_VALUE_FUNC(uint8_t, bm_l2_get_num_ports);
DECLARE_FAKE_VALUE_FUNC(uint8_t, bm_l2_get_num_devices);
DECLARE_FAKE_VALUE_FUNC(bool, bm_l2_get_port_state, uint8_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_l2_netif_set_power, void *, bool);
