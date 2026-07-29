#include "mock_resource_based_routing.h"

DEFINE_FAKE_VALUE_FUNC(BmErr, routing_init);
DEFINE_FAKE_VALUE_FUNC(BmErr, add_local_resource, const char *, BmTopicLength,
                       ResourceId *);
DEFINE_FAKE_VALUE_FUNC(BmErr, add_neighbor_resource, const char *,
                       BmTopicLength, ResourceId *, uint8_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, remove_local_resource, const char *,
                       BmTopicLength);
DEFINE_FAKE_VALUE_FUNC(BmErr, remove_neighbor_resource, const char *,
                       BmTopicLength, uint8_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, get_forward_port_mask, ResourceId *, uint8_t,
                       uint16_t *, bool *, ResourceOptions);
DEFINE_FAKE_VALUE_FUNC(BmErr, get_topic_element, const char *, BmTopicLength,
                       ResourceTrieElement *);
