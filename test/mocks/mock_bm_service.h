#include "bm_service.h"
#include "fff.h"

DECLARE_FAKE_VALUE_FUNC(BmErr, bm_service_init);
DECLARE_FAKE_VALUE_FUNC(bool, bm_service_register, size_t, const char *,
                        BmServiceHandler);
DECLARE_FAKE_VALUE_FUNC(bool, bm_service_unregister, size_t, const char *);
