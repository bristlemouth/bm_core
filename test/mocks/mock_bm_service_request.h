#include "bm_service_request.h"
#include "fff.h"

DECLARE_FAKE_VOID_FUNC(bm_service_request_init);
DECLARE_FAKE_VALUE_FUNC(bool, bm_service_request, size_t, const char *, size_t,
                        const uint8_t *, BmServiceReplyCb, uint32_t);
