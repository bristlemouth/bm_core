#include "fff.h"
#include "power_info_reply_msg.h"
#include "power_info_service.h"

DECLARE_FAKE_VALUE_FUNC(BmErr, power_info_service_init, BmPowerInfoStatsCb,
                        void *);
DECLARE_FAKE_VALUE_FUNC(BmErr, power_info_service_request, BmPowerInfoReplyCb,
                        uint32_t);
