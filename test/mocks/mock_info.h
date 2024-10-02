#include "fff.h"
#include "util.h"

typedef void (*RequestInfoCb)(void *);

DECLARE_FAKE_VOID_FUNC(bcmp_expect_info_from_node_id, uint64_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, bcmp_request_info, uint64_t, const void *,
                        RequestInfoCb);
DECLARE_FAKE_VALUE_FUNC(BmErr, bcmp_device_info_init);
