#include "fff.h"
#include "resource_trie.h"

DECLARE_FAKE_VALUE_FUNC(BmErr, resource_trie_add, ResourceTrieRoot *,
                        const char *, BmTopicLength, uint32_t, uint16_t, bool);
DECLARE_FAKE_VALUE_FUNC(BmErr, resource_trie_match, ResourceTrieRoot *,
                        const char *, BmTopicLength);
DECLARE_FAKE_VALUE_FUNC(BmErr, resource_trie_match_exact, ResourceTrieRoot *,
                        const char *, BmTopicLength);
DECLARE_FAKE_VALUE_FUNC(BmErr, resource_trie_remove, ResourceTrieRoot *,
                        const char *, BmTopicLength);
DECLARE_FAKE_VALUE_FUNC(BmErr, resource_trie_purge, ResourceTrieRoot *);
