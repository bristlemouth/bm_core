#include "fff.h"
#include "messages/resource_discovery.h"
#include "util.h"

DECLARE_FAKE_VALUE_FUNC(BmErr, bcmp_resource_discovery_init);
DECLARE_FAKE_VALUE_FUNC(BmErr, bcmp_resource_discovery_add_resource,
                        const char *, const uint16_t, ResourceType, uint32_t);
