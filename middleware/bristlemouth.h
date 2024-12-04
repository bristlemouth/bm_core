#include "device.h"
#include "network_device.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*NetworkDevicePowerCallback)(bool);

BmErr bristlemouth_init(NetworkDevicePowerCallback net_power_cb,
                        DeviceCfg device);
NetworkDevice bristlemouth_network_device(void);
BmErr bristlemouth_handle_network_device_interrupt(void);

#ifdef __cplusplus
}
#endif
