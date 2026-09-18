#include "mock_resource_trie.h"

DEFINE_FAKE_VALUE_FUNC(BmErr, resource_trie_add, ResourceTrieRoot *,
                       const char *, BmTopicLength, uint32_t, uint16_t, bool);
DEFINE_FAKE_VALUE_FUNC(BmErr, resource_trie_match, ResourceTrieRoot *,
                       const char *, BmTopicLength);
DEFINE_FAKE_VALUE_FUNC(BmErr, resource_trie_match_exact, ResourceTrieRoot *,
                       const char *, BmTopicLength);
DEFINE_FAKE_VALUE_FUNC(BmErr, resource_trie_remove, ResourceTrieRoot *,
                       const char *, BmTopicLength);
DEFINE_FAKE_VALUE_FUNC(BmErr, resource_trie_purge, ResourceTrieRoot *);
