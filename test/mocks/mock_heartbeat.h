#include "fff.h"
#include "util.h"
#include <stdint.h>

DECLARE_FAKE_VALUE_FUNC(BmErr, bcmp_heartbeat_init);
DECLARE_FAKE_VALUE_FUNC(BmErr, bcmp_send_heartbeat, uint32_t);
