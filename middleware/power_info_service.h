#ifndef __POWER_INFO_SERVICE_H__
#define __POWER_INFO_SERVICE_H__

#include "power_info_reply_msg.h"
#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef BmErr (*BmPowerInfoReplyCb)(const PowerInfoReplyData *d);
typedef PowerInfoReplyData (*BmPowerInfoStatsCb)(void *arg);

BmErr power_info_service_init(BmPowerInfoStatsCb power_cb, void *arg);
BmErr power_info_service_request(BmPowerInfoReplyCb reply_cb,
                                 uint32_t timeout_s);

#ifdef __cplusplus
}
#endif

#endif
