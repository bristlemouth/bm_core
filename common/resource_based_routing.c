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

/*!
 @brief Initialize resource based routing

 @details Creates a hash table for each port on the device. Each entry in the
          table uses the neighbor's associated resource id with a given topic
          as the index of the data. The data points to an element in the
          resource_trie. This provides fast resource ID lookup when forwarding
          and ingesting potential packets of interest.

 @return BmOK on success
         BmENOMEM if there is not enough memory to allocate for the hash tables
 */
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

void routing_cleanup(void) {
  uint8_t num_ports = bm_l2_get_port_count();

  // Free resources if they failed to allocate
  for (uint8_t port_free = 0; port_free < num_ports; port_free++) {
    hash_delete(ctx.hash[port_free]);
  }
  bm_free(ctx.hash);
}

/*!
 @brief Add a local resource of interest

 @details Attempts to find a resource within the resource trie
          and if it exists, will update the local_interest flag of
          the element. If the resource does not exist, will add a new
          one to the trie.

 @param topic string representing the topic of the resource of interest
 @param len length of topic
 @param resource_id output resource ID assigned to the element

 @return BmOK on success
         BmErr on failure
 */
BmErr add_local_resource(const char *topic, BmTopicLength len,
                         ResourceId *resource_id) {
  if (!topic || !len || !resource_id) {
    return BmEINVAL;
  }

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

/*!
 @brief Add a neighbor resource

 @details Attempts to find find a matching resource in the trie.
          If not found, will add a new element to the trie. The port_mask is
          updated with the ingress port the neighbor's information was received
          on. Will also add a hash table element.

 @param topic topic string neighbor is requesting to share
 @param len length of topic string
 @param resource_id input resource ID from neighbor, output local resource ID assigned to element
 @param port_num ingress port number the information was received on

 @return BmOK on success
         BmErr on failure
 */
BmErr add_neighbor_resource(const char *topic, BmTopicLength len,
                            ResourceId *resource_id, uint8_t port_num) {
  if (!port_num || port_num > bm_l2_get_port_count() || !topic || !len ||
      !resource_id) {
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

  Hash *hash = ctx.hash[port_num - 1];
  err = hash_look_up(hash, *resource_id, &element);

  if (err == BmOK) {
    // Remove from the hash table if the element does exist
    bm_err_report(err, hash_remove(hash, *resource_id));
  } else {
    // couldn't be found in hash table, no problem
    err = BmOK;
  }
  bm_err_check(err, hash_insert(hash, *resource_id, &element));
  if (err != BmOK) {
    return err;
  }

  // Update port mask if added to hash table correctly
  element->port_mask |= port_mask;

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
    bm_err_report(err, hash_remove(hash, element->resource_id));
  }

  if (err != BmOK) {
    return err;
  }

  if (element->port_mask || element->local_interest) {
    return BmOK;
  }

  // Remove from the trie there is no longer any interest
  bm_err_check(err, resource_trie_remove(&ctx.trie, topic, len));
  return err;
}

/*!
 @brief Remove a resource of local interest

 @details Will remove the element from the resource trie if there are no
          forwarding ports, otherwise just clears the local_interest flag.

 @param topic topic string to remove
 @param len length of topic string

 @return BmOK on success
         BmErr on failure
 */
BmErr remove_local_resource(const char *topic, BmTopicLength len) {
  if (!topic || !len) {
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

/*!
 @brief Remove a neighbors resource interest

 @details Will only remove the element from the resource trie if there are no
          other forwarding ports and there is not a local interest. Will also
          remove the resource ID from the associated hash table.

 @param topic topic string neighbor is requesting to remove
 @param len length of topic string
 @param port_num port number the neighbor is on

 @return BmOK on success
         BmErr on failure
 */
BmErr remove_neighbor_resource(const char *topic, BmTopicLength len,
                               uint8_t port_num) {
  if (!port_num || port_num > bm_l2_get_port_count() || !len || !topic) {
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

/*!
 @brief Obtains forwarding port mask and interest information

 @details Finds resource trie element via packet ingress port's hash table adn
          determines which output ports the packet should be forwarded too.
          Also determines if there is a local interest in the packet.

 @param resource_id input neighbor assigned resource ID, output local assigned
                    resource ID
 @param port_num ingress port the packet was received on
 @param forward_mask output port mask to forward packet on
 @param local_interest output local interest 
 @param opts resource option from message

 @return BmOK on success
         BmErr on failure
 */
BmErr get_forward_port_mask(ResourceId *resource_id, uint8_t port_num,
                            uint16_t *forward_mask, bool *local_interest,
                            ResourceOptions opts) {
  //TODO: Implement options once req/reply is ready
  (void)opts;

  if (!port_num || port_num > bm_l2_get_port_count() || !forward_mask ||
      !resource_id || !local_interest) {
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

/*!
 @brief Obtain resource trie element from topic sting 

 @param topic topic string to obtain element from
 @param len length of topic string
 @param element resource trie element

 @return BmOK on success
         BmErr on failure
 */
BmErr get_topic_element(const char *topic, BmTopicLength len,
                        ResourceTrieElement *element) {
  if (!topic || !len || !element) {
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
