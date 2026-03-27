#include "pubsub.h"
#include "util.h"

// The maximum depth a resource trie will be if every topic segment
// is a single character:
//                     x/x/x/x/x......./x
#define resource_trie_max_depth (BM_TOPIC_MAX_LEN >> 1)

// Max resource key is 20 bit
#define max_resource_id (0xFFFFF)
#define invalid_resource_id UINT32_MAX

typedef struct ResourceTrieElement {
  struct ResourceTrieElement *children; // Children from this pattern
  struct ResourceTrieElement *sibling;  // Next sibling under same parent
  struct ResourceTrieElement *match;    // Next in circular match ring
  const char *segment;                  // String segment key
  uint32_t resource_id;                 // Local resource ID
  uint16_t port_mask; // Mask of ports which expressed interest in this resource
  uint16_t local_interest : 1; // Does the node subscribe to this resource
  uint16_t reserved : 15;      // Maintains alignment
} ResourceTrieElement;

typedef struct {
  ResourceTrieElement **matches; // Caller-supplied array.
  uint16_t capacity;             // Maximum results to record.
  uint16_t count;                // Number of results recorded.
} ResourceTrieMatchResult;

typedef struct {
  ResourceTrieElement *element;
  const char *remaining;
} ResourceTrieStack;

typedef struct {
  ResourceTrieElement element;
  ResourceTrieStack stack[resource_trie_max_depth];
} ResourceTrieRoot;

BmErr resource_trie_add(ResourceTrieRoot *root, const char *topic,
                        uint32_t resource_id, uint16_t port_mask,
                        bool local_interest);
BmErr resource_trie_match(ResourceTrieRoot *root, const char *topic,
                          ResourceTrieMatchResult *result);
BmErr resource_trie_remove(ResourceTrieRoot *root, const char *topic);
