#include "network_device.h"

typedef void (*NetworkDevicePowerCallback)(bool);

BmErr bristlemouth_init(NetworkDevicePowerCallback net_power_cb);
NetworkDevice bristlemouth_network_device(void);
