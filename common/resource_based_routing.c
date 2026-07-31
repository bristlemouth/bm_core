#include "resource_based_routing.h"
#include "bm_os.h"
#include "hash.h"
#include "l2.h"

#define DEFAULT_HASH_TABLE_SIZE 512

static struct {
  ResourceTrieRoot trie;
  Hash **hash;
  uint32_t local_id;
} ctx = {0};

BmErr routing_init(void) {
  uint8_t num_ports = bm_l2_get_port_count();
  ctx.hash = bm_malloc(num_ports * sizeof(Hash *));
  if (!ctx.hash) {
    return BmENOMEM;
  }

  BmErr err = BmOK;
  uint8_t port = 0;
  for (; port < num_ports; port++) {
    ctx.hash[port] =
        hash_create(sizeof(ResourceTrieElement *), DEFAULT_HASH_TABLE_SIZE);
    if (!ctx.hash[port]) {
      err = BmENOMEM;
      break;
    }
  }

  if (err == BmOK) {
    return err;
  }

  // Free resources if they failed to allocate
  for (uint8_t port_free = 0; port_free < port; port_free++) {
    hash_delete(ctx.hash[port_free]);
  }
  bm_free(ctx.hash);

  return err;
}

BmErr add_local_resource(const char *topic, BmTopicLength len,
                         ResourceId *resource_id) {
  BmErr err = resource_trie_match_exact(&ctx.trie, topic, len);
  if (err != BmOK) {
    return err;
  }

  if (!ctx.trie.result.count) {
    err = resource_trie_add(&ctx.trie, topic, len, ctx.local_id, 0, true);
    if (err != BmOK) {
      return err;
    }
    *resource_id = ctx.local_id;
    ctx.local_id++;
  } else {
    ResourceTrieElement *element = ctx.trie.result.matches[0];
    element->local_interest = true;
    *resource_id = element->resource_id;
  }

  return BmOK;
}

BmErr add_neighbor_resource(const char *topic, BmTopicLength len,
                            ResourceId *resource_id, uint8_t port_num) {
  if (!port_num || port_num > bm_l2_get_port_count() || !topic) {
    return BmEINVAL;
  }

  uint16_t port_mask = 1 << (port_num - 1);
  BmErr err = resource_trie_match_exact(&ctx.trie, topic, len);
  if (err != BmOK) {
    return err;
  }

  if (!ctx.trie.result.count) {
    err = resource_trie_add(&ctx.trie, topic, len, ctx.local_id, port_mask,
                            false);
    if (err != BmOK) {
      return err;
    }
    ctx.local_id++;
  }

  ResourceTrieElement *element = ctx.trie.result.matches[0];
  element->port_mask |= port_mask;

  // Add to the hash table
  Hash *hash = ctx.hash[port_num - 1];
  err = hash_insert(hash, *resource_id, &element);

  // If neighbor changed resource id for whatever reason, that is ok
  if (err == BmEALREADY) {
    err = BmOK;
  }

  // Set the output resource ID now
  *resource_id = element->resource_id;

  return err;
}

static BmErr remove_from_data_structs(const char *topic, BmTopicLength len,
                                      uint8_t port_num,
                                      ResourceTrieElement *element) {
  BmErr err = BmOK;

  if (port_num) {
    Hash *hash = ctx.hash[port_num - 1];
    err = hash_remove(hash, element->resource_id);
  }

  // Remove from the trie there is no longer any interest
  if (element->port_mask || element->local_interest) {
    return BmOK;
  }

  bm_err_check(err, resource_trie_remove(&ctx.trie, topic, len));
  return err;
}

BmErr remove_local_resource(const char *topic, BmTopicLength len) {
  if (!topic) {
    return BmEINVAL;
  }

  BmErr err = resource_trie_match_exact(&ctx.trie, topic, len);
  if (err != BmOK) {
    return err;
  }

  if (!ctx.trie.result.count) {
    return BmENODATA;
  }

  ResourceTrieElement *element = ctx.trie.result.matches[0];
  element->local_interest = false;

  return remove_from_data_structs(topic, len, 0, element);
}

BmErr remove_neighbor_resource(const char *topic, BmTopicLength len,
                               uint8_t port_num) {
  if (!port_num || port_num > bm_l2_get_port_count() || !topic) {
    return BmEINVAL;
  }

  uint16_t port_mask = 1 << (port_num - 1);
  BmErr err = resource_trie_match_exact(&ctx.trie, topic, len);
  if (err != BmOK) {
    return err;
  }

  if (!ctx.trie.result.count) {
    return BmENODATA;
  }

  ResourceTrieElement *element = ctx.trie.result.matches[0];
  element->port_mask &= ~port_mask;

  return remove_from_data_structs(topic, len, port_num, element);
}

BmErr get_forward_port_mask(ResourceId *resource_id, uint8_t port_num,
                            uint16_t *forward_mask, bool *local_interest,
                            ResourceOptions opts) {
  //TODO: Implement options once req/reply is ready
  (void)opts;

  if (!port_num || port_num > bm_l2_get_port_count() || !forward_mask) {
    return BmEINVAL;
  }

  Hash *hash = ctx.hash[port_num - 1];
  ResourceTrieElement *element;
  BmErr err = hash_look_up(hash, *resource_id, &element);
  if (err != BmOK) {
    return err;
  }

  // Do not forward on the incoming port
  uint16_t port_mask = 1 << (port_num - 1);
  *forward_mask =
      (element->port_mask | element->wildcard_port_mask) & ~port_mask;
  *local_interest = element->local_interest | element->wildcard_interest;
  *resource_id = element->resource_id;

  return BmOK;
}

BmErr get_topic_element(const char *topic, BmTopicLength len,
                        ResourceTrieElement *element) {
  if (!topic || !element) {
    return BmEINVAL;
  }

  BmErr err = resource_trie_match_exact(&ctx.trie, topic, len);
  if (err != BmOK) {
    return err;
  }

  if (!ctx.trie.result.count) {
    return BmENODATA;
  }

  memcpy(element, ctx.trie.result.matches[0], sizeof(ResourceTrieElement));

  return BmOK;
}
