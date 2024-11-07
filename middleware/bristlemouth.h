#include "bm_adin2111.h"

typedef void (*NetworkDevicePowerCallback)(bool);

BmErr bristlemouth_init(NetworkDevicePowerCallback net_power_cb);
