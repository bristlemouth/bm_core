#include "mock_power_info_service.h"

DEFINE_FAKE_VALUE_FUNC(BmErr, power_info_service_init, BmPowerInfoStatsCb,
                       void *);
DEFINE_FAKE_VALUE_FUNC(BmErr, power_info_service_request, BmPowerInfoReplyCb,
                       uint32_t);
