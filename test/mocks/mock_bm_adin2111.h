#include "bm_adin2111.h"
#include "fff.h"

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_FAKE_VOID_FUNC(network_device_power_cb, bool);
DECLARE_FAKE_VOID_FUNC(link_changed_on_port, uint8_t, bool);
DECLARE_FAKE_VALUE_FUNC(size_t, received_data_on_port, uint8_t, uint8_t *,
                        size_t);

DECLARE_FAKE_VALUE_FUNC(BmErr, netdevice_send, void *, uint8_t *, size_t,
                        uint8_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, netdevice_enable, void *);
DECLARE_FAKE_VALUE_FUNC(BmErr, netdevice_disable, void *);

NetworkDeviceCallbacks const fake_netdevice_callbacks = {
    .power = network_device_power_cb,
    .link_change = link_changed_on_port,
    .receive = received_data_on_port};

NetworkDevice create_mock_network_device(void);

#ifdef __cplusplus
}
#endif
