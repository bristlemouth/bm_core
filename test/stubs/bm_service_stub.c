#include "mock_bm_service.h"

DEFINE_FAKE_VOID_FUNC(bm_service_init);
DEFINE_FAKE_VALUE_FUNC(bool, bm_service_register, size_t, const char *,
                       BmServiceHandler);
DEFINE_FAKE_VALUE_FUNC(bool, bm_service_unregister, size_t, const char *);
