#include "fff.h"
#include "resource_based_routing.h"

DECLARE_FAKE_VALUE_FUNC(BmErr, routing_init);
DECLARE_FAKE_VALUE_FUNC(BmErr, add_local_resource, const char *, BmTopicLength,
                        ResourceId *);
DECLARE_FAKE_VALUE_FUNC(BmErr, add_neighbor_resource, const char *,
                        BmTopicLength, ResourceId *, uint8_t, bool);
DECLARE_FAKE_VALUE_FUNC(BmErr, remove_local_resource, const char *,
                        BmTopicLength);
DECLARE_FAKE_VALUE_FUNC(BmErr, remove_neighbor_resource, const char *,
                        BmTopicLength, uint8_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, get_forward_port_mask, ResourceId *, uint8_t,
                        uint16_t *, bool *, ResourceOptions);
DECLARE_FAKE_VALUE_FUNC(BmErr, get_topic_element, const char *, BmTopicLength,
                        ResourceTrieElement *);
