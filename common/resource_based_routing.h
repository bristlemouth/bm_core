#include "resource_trie.h"
#include "util.h"

// Reference figure 5.3 in Bristlemouth spec which describes the resource
// options in a packet
typedef struct {
  uint32_t timeout : 4;
  uint32_t request_id : 12;
  uint32_t type : 2;
  uint32_t res : 2;
  uint32_t : 12;
} ResourceOptions;

typedef enum {
  option_pubsub = 0x0,
  option_request = 0x1,
  option_reply = 0x3,
} ResourceOptionsType;

typedef enum {
  option_timeout_100ms,
  option_timeout_200ms,
  option_timeout_300ms,
  option_timeout_400ms,
  option_timeout_500ms,
  option_timeout_1s,
  option_timeout_2s,
  option_timeout_4s,
  option_timeout_8s,
  option_timeout_16s,
  option_timeout_32s,
  option_timeout_64s,
  option_timeout_128s,
  option_timeout_256s,
  option_timeout_512s,
} ResourceOptionsTimeout;

typedef uint32_t ResourceId;

BmErr routing_init(void);
void routing_cleanup(void);
BmErr add_local_resource(const char *topic, BmTopicLength len,
                         ResourceId *resource_id);
BmErr add_neighbor_resource(const char *topic, BmTopicLength len,
                            ResourceId *resource_id, uint8_t port_num);
BmErr remove_local_resource(const char *topic, BmTopicLength len);
BmErr remove_neighbor_resource(const char *topic, BmTopicLength len,
                               uint8_t port_num);
BmErr get_forward_port_mask(ResourceId *resource_id, uint8_t port_num,
                            uint16_t *forward_mask, bool *local_interest,
                            ResourceOptions opts);
BmErr get_topic_element(const char *topic, BmTopicLength len,
                        ResourceTrieElement *element);
