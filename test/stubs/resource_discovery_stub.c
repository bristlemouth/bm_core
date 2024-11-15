#include "mock_resource_discovery.h"

DEFINE_FAKE_VALUE_FUNC(BmErr, bcmp_resource_discovery_init);
DEFINE_FAKE_VALUE_FUNC(BmErr, bcmp_resource_discovery_add_resource,
                       const char *, const uint16_t, ResourceType, uint32_t);
