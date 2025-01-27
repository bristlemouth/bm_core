#include "bm_adin2111.h"
#include "fff.h"

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_FAKE_VOID_FUNC(network_device_power_cb, bool);
DECLARE_FAKE_VOID_FUNC(link_changed_on_port, uint8_t, bool);
DECLARE_FAKE_VOID_FUNC(received_data_on_port, uint8_t, uint8_t *, size_t);

DECLARE_FAKE_VALUE_FUNC(BmErr, netdevice_send, void *, uint8_t *, size_t,
                        uint8_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, netdevice_enable, void *);
DECLARE_FAKE_VALUE_FUNC(BmErr, netdevice_disable, void *);
DECLARE_FAKE_VALUE_FUNC(BmErr, netdevice_enable_port, void *, uint8_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, netdevice_disable_port, void *, uint8_t);
DECLARE_FAKE_VALUE_FUNC(uint8_t, netdevice_num_ports);
DECLARE_FAKE_VALUE_FUNC(BmErr, netdevice_port_stats, void *, uint8_t, void *);

NetworkDevice create_mock_network_device(void);

#ifdef __cplusplus
}
#endif
