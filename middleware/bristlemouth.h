#include "device.h"
#include "network_device.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*NetworkDevicePowerCallback)(bool);

BmErr bristlemouth_init(NetworkDevicePowerCallback net_power_cb,
                        DeviceCfg device);
NetworkDevice bristlemouth_network_device(void);

#ifdef __cplusplus
}
#endif
