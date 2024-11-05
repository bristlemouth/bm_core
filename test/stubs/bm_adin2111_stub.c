#include "fff.h"
#include "mock_bm_adin2111.h"

DEFINE_FFF_GLOBALS;

DEFINE_FAKE_VOID_FUNC(network_device_power_cb, bool);
DEFINE_FAKE_VOID_FUNC(link_changed_on_port, uint8_t, bool);
DEFINE_FAKE_VOID_FUNC(received_data_on_port, uint8_t, uint8_t *, size_t);

DEFINE_FAKE_VALUE_FUNC(BmErr, netdevice_send, void *, uint8_t *, size_t,
                       uint8_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, netdevice_enable, void *);
DEFINE_FAKE_VALUE_FUNC(BmErr, netdevice_disable, void *);

static NetworkDeviceTrait const fake_netdevice_trait = {
    .send = netdevice_send,
    .enable = netdevice_enable,
    .disable = netdevice_disable};

NetworkDevice create_mock_network_device(void) {
  return (NetworkDevice){.trait = &fake_netdevice_trait,
                         .callbacks = fake_netdevice_callbacks};
}
