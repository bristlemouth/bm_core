#include "mock_bm_service_request.h"

DEFINE_FAKE_VALUE_FUNC(BmErr, bm_service_request_init);
DEFINE_FAKE_VALUE_FUNC(bool, bm_service_request, size_t, const char *, size_t,
                       const uint8_t *, BmServiceReplyCb, uint32_t);
