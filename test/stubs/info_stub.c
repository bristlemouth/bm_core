#include "mock_info.h"

DEFINE_FAKE_VOID_FUNC(bcmp_expect_info_from_node_id, uint64_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, bcmp_request_info, uint64_t, const void *,
                       RequestInfoCb);
DEFINE_FAKE_VALUE_FUNC(BmErr, bcmp_device_info_init);
