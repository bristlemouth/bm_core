#include "messages.h"
#include "util.h"
#include <stdint.h>

#define default_resource_add_timeout_ms (100)

typedef enum { PUB, SUB } ResourceType;

BmErr bcmp_resource_discovery_init(void);
BmErr bcmp_resource_discovery_add_resource(const char *res,
                                           const uint16_t resource_len,
                                           ResourceType type,
                                           uint32_t timeoutMs);
BmErr bcmp_resource_discovery_get_num_resources(uint16_t *num_resources,
                                                ResourceType type,
                                                uint32_t timeoutMs);
BmErr bcmp_resource_discovery_find_resource(const char *res,
                                            const uint16_t resource_len,
                                            bool *found, ResourceType type,
                                            uint32_t timeoutMs);
BmErr bcmp_resource_discovery_send_request(uint64_t target_node_id,
                                           void (*cb)(void *));
void bcmp_resource_discovery_print_resources(void);
BcmpResourceTableReply *bcmp_resource_discovery_get_local_resources(void);
