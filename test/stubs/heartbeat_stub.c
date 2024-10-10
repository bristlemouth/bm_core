#include "mock_heartbeat.h"

DEFINE_FAKE_VALUE_FUNC(BmErr, bcmp_heartbeat_init);
DEFINE_FAKE_VALUE_FUNC(BmErr, bcmp_send_heartbeat, uint32_t);
