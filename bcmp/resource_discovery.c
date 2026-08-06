#include "messages/resource_discovery.h"
#include "bcmp.h"
#include "bm_config.h"
#include "bm_os.h"
#include "device.h"
#include "l2.h"
#include "ll.h"
#include "packet.h"
#include "resource_based_routing.h"
#include "string.h"
#include <inttypes.h>
#include <stdlib.h>

typedef struct BcmpResourceNode {
  BcmpResource *resource;
  struct BcmpResourceNode *next;
} BcmpResourceNode;

typedef struct BcmpResourceList {
  struct BcmpResourceNode *start;
  struct BcmpResourceNode *end;
  uint16_t num_resources;
  BmSemaphore lock;
} BcmpResourceList;

typedef struct {
  void (*cb)(void *);
} ResourceCb;

static BcmpResourceList PUB_LIST;
static BcmpResourceList SUB_LIST;
static LL RESOURCE_REQUEST_LIST;

// TODO: Make this a table for faster lookup.
static bool bcmp_resource_discovery_find_resource_priv(
    const char *resource, const uint16_t resource_len, ResourceType type) {
  bool rval = false;
  BcmpResourceList *res_list = (type == SUB) ? &SUB_LIST : &PUB_LIST;
  do {
    BcmpResourceNode *cur = res_list->start;
    while (cur) {
      if (memcmp(resource, cur->resource->resource, resource_len) == 0) {
        return true;
      }
      cur = cur->next;
    }
  } while (0);
  return rval;
}

static bool bcmp_resource_compute_list_size(ResourceType type,
                                            size_t *msg_len) {
  bool rval = false;
  BcmpResourceList *res_list = (type == SUB) ? &SUB_LIST : &PUB_LIST;
  if (bm_semaphore_take(res_list->lock, default_resource_add_timeout_ms) ==
      BmOK) {
    do {
      BcmpResourceNode *cur = res_list->start;
      while (cur != NULL) {
        *msg_len += (sizeof(BcmpResource) + cur->resource->resource_len);
        cur = cur->next;
      }
      rval = true;
    } while (0);
    bm_semaphore_give(res_list->lock);
  }
  return rval;
}

static bool bcmp_resource_populate_msg_data(ResourceType type,
                                            BcmpResourceTableReply *repl,
                                            uint32_t *data_offset) {
  bool rval = false;
  BcmpResourceList *res_list = (type == SUB) ? &SUB_LIST : &PUB_LIST;
  if (bm_semaphore_take(res_list->lock, default_resource_add_timeout_ms) ==
      BmOK) {
    do {
      if (type == PUB) {
        repl->num_pubs = res_list->num_resources;
      } else { // SUB
        repl->num_subs = res_list->num_resources;
      }
      BcmpResourceNode *cur = res_list->start;
      while (cur != NULL) {
        size_t res_size = (sizeof(BcmpResource) + cur->resource->resource_len);
        memcpy(&repl->resource_list[*data_offset], cur->resource, res_size);
        *data_offset += res_size;
        cur = cur->next;
      }
      rval = true;
    } while (0);
    bm_semaphore_give(res_list->lock);
  }
  return rval;
}

/*!
  Process the resource discovery request message.

  \param in data - request data
  \return - BmOK if successful, an error otherwise
*/
static BmErr bcmp_process_resource_discovery_request(BcmpProcessData data) {
  BmErr err = BmEBADMSG;
  BcmpResourceTableRequest *req = (BcmpResourceTableRequest *)data.payload;
  do {
    if (req->target_node_id != node_id()) {
      break;
    }
    size_t msg_len = sizeof(BcmpResourceTableReply);
    if (!bcmp_resource_compute_list_size(PUB, &msg_len)) {
      bm_debug("Failed to get publishers list\n.");
      break;
    }
    if (!bcmp_resource_compute_list_size(SUB, &msg_len)) {
      bm_debug("Failed to get subscribers list\n.");
      break;
    }

    // Create and fill the reply
    uint8_t *reply_buf = (uint8_t *)bm_malloc(msg_len);
    if (reply_buf == NULL) {
      err = BmENOMEM;
      break;
    }
    BcmpResourceTableReply *reply = (BcmpResourceTableReply *)reply_buf;
    reply->node_id = node_id();
    uint32_t data_offset = 0;
    if (!bcmp_resource_populate_msg_data(PUB, reply, &data_offset)) {
      bm_debug("Failed to get publishers list\n.");
      break;
    }
    if (!bcmp_resource_populate_msg_data(SUB, reply, &data_offset)) {
      bm_debug("Failed to get publishers list\n.");
      break;
    }

    err = bcmp_tx(data.dst, BcmpResourceTableReplyMessage, reply_buf, msg_len,
                  0, NULL);
    if (err != BmOK) {
      bm_debug("Failed to send bcmp resource table reply, error %d\n", err);
    };
    bm_free(reply_buf);
  } while (0);

  return err;
}

/*!
  @brief Process the resource discovery reply message.

  @param in *repl - reply
  @param in src_node_id - node ID of the source.

  @return BmOK if message found or this message is not for us
  @return BmErr if unsuccessful
*/
static BmErr bcmp_process_resource_discovery_reply(BcmpProcessData data) {
  BmErr err = BmOK;
  BcmpResourceTableReply *repl = (BcmpResourceTableReply *)data.payload;
  uint64_t src_node_id = ip_to_nodeid(data.src);
  ResourceCb *cb = NULL;

  if (repl->node_id == src_node_id) {
    err = BmEBADMSG;
    err = ll_get_item(&RESOURCE_REQUEST_LIST, src_node_id, (void **)&cb);
    if (err == BmOK && cb->cb != NULL) {
      cb->cb(repl);
    } else if (err == BmOK) {
      bm_debug("Node Id %016" PRIx64 " resource table:\n", src_node_id);
      uint16_t num_pubs = repl->num_pubs;
      size_t offset = 0;
      bm_debug("\tPublishers:\n");
      while (num_pubs) {
        BcmpResource *cur_resource =
            (BcmpResource *)&repl->resource_list[offset];
        bm_debug("\t* %.*s\n", cur_resource->resource_len,
                 cur_resource->resource);
        offset += (sizeof(BcmpResource) + cur_resource->resource_len);
        num_pubs--;
      }
      uint16_t num_subs = repl->num_subs;
      bm_debug("\tSubscribers:\n");
      while (num_subs) {
        BcmpResource *cur_resource =
            (BcmpResource *)&repl->resource_list[offset];
        bm_debug("\t* %.*s\n", cur_resource->resource_len,
                 cur_resource->resource);
        offset += (sizeof(BcmpResource) + cur_resource->resource_len);
        num_subs--;
      }
    }
    ll_remove(&RESOURCE_REQUEST_LIST, src_node_id);
  }

  return err;
}

/*!
 @brief Determine if resource info is valid

 @details Compares info struct to the size reported from the data packet.

 @param info resource info received 
 @param size sizeo of data packet

 @return true if packet size matches expectations
         false otherwise
 */
static bool resource_info_valid(const ResourceInfo *info, uint32_t size) {
  // Bounds checking on the message
  if (size < sizeof(ResourceInfo) || info->length >= BM_TOPIC_MAX_LEN ||
      info->length + sizeof(ResourceInfo) != size) {
    return false;
  }

  return true;
}

/*!
 @brief Handle a resource request message

 @details Adds the resource info to the routing table.

 @param payload incoming payload from replying device

 @return BmOK on success
         BmENODATA if full packet is not able to be retrieved from packet
                   module
         BmEBADMSG if the packet information is not valid
         BmErr on failure to add resource
 */
static BmErr resource_discovery_reply_cb(uint8_t *payload) {
  ResourceInfo *rep = (ResourceInfo *)payload;
  if (!rep) {
    return BmEINVAL;
  }

  uint32_t id = rep->resource_id;
  const BcmpProcessData *data = packet_get_data();
  if (!data) {
    return BmENODATA;
  }

  if (!resource_info_valid(rep, data->size)) {
    return BmEBADMSG;
  }

  return add_neighbor_resource(rep->topic, rep->length, &id,
                               data->ingress_port);
}

/*!
 @brief Adds to resource table and updates the resource ID to locally assigned

 @details This is meant to add the resource for routing purposes and then
          update the resource's ID to be used with propogated requests on other
          ports and as a reply to the original requestor to inform those nodes
          of the ID assigned to this node's resource.

 @param info resource information to save and update the resource ID on
 @param port_num ingress port number the resource was shared on

 @return BmOK on success
         BmErr on failure
 */
static BmErr add_resource_update_id(ResourceInfo *info, uint8_t port_num) {
  uint32_t id = info->resource_id;
  BmErr err = add_neighbor_resource(info->topic, info->length, &id, port_num);

  if (err == BmOK) {
    info->resource_id = id;
  }

  return err;
}

/*!
 @brief Reply to resource request

 @details Prepares reply message to send to requesting device.
          add_resource_update_id must be invoked before this function.

 @param data data from request with updated resource ID

 @return BmOK on success
         BmErr on failure
 */
static BmErr resource_reply(BcmpProcessData data) {
  uint8_t *rep = data.payload;
  uint16_t rep_size = data.size;
  uint32_t seq_num = data.header->seq_num;
  uint8_t port_num = data.ingress_port;

  BcmpTxCtx ctx = {
      .dst = data.dst,
      .type = BcmpResourceReplyMessage,
      .data = rep,
      .size = rep_size,
      .seq_num = seq_num,
      .reply_cb = NULL,
      .egress_port = port_num,
  };

  BmErr err;
  bm_err_report(err, bcmp_tx_port(ctx));

  return err;
}

/*!
 @brief Propogate resource request to other nodes on network

 @details Will request to send information info to all other online ports on
          the network. add_resource_update_id must be invoked before this 
          function.

 @param data data from request with updated resource ID

 @return BmOK on success
         BmEBADMSG if the packet information is not valid
         BmErr on failure
 */
static BmErr resource_propogate(BcmpProcessData data) {
  uint8_t *rep = data.payload;
  uint16_t rep_size = data.size;
  uint8_t ingress_port = data.ingress_port;

  uint8_t num_ports = bm_l2_get_port_count();
  BcmpTxCtx ctx = {
      .dst = data.dst,
      .type = BcmpResourceRequestMessage,
      .data = rep,
      .size = rep_size,
      .seq_num = 0,
      .reply_cb = resource_discovery_reply_cb,
      .egress_port = 1,
  };

  // Send request to all ports besides ingress port and offline ports
  for (; ctx.egress_port <= num_ports; ctx.egress_port++) {
    if (ctx.egress_port == ingress_port ||
        !bm_l2_get_port_state(ctx.egress_port - 1)) {
      continue;
    }
    BmErr tx_err = bcmp_tx_port(ctx);
    if (tx_err != BmOK) {
      bm_debug("Failed to forward bcmp resource request on port %u, error %d\n",
               ctx.egress_port, tx_err);
    };
  }

  return BmOK;
}

/*!
 @brief Process an incoming resource request

 @details Add requested resource to resource table. Also sends out requests to
          other ports to propogate the resource of interest onto the network.

 @param data

 @return BmOK on success
         
         BmErr on failure
 */
static BmErr bcmp_process_resource_request(BcmpProcessData data) {
  ResourceInfo *req = (ResourceInfo *)data.payload;
  if (!resource_info_valid(req, data.size)) {
    return BmEBADMSG;
  }

  // Copy over data to reply and let callback manipulate fields in reply
  uint8_t port_num = data.ingress_port;
  BmErr err;
  bm_err_report(err, add_resource_update_id(req, port_num));

  // Reply with the information needed from the requestor
  bm_err_check(err, resource_reply(data));

  // Propogate resource information down the network
  bm_err_check(err, resource_propogate(data));

  return err;
}

/*!
  @brief Init the bcmp resource discovery module.
*/
BmErr bcmp_resource_discovery_init(void) {
  BmErr err = BmENOMEM;
  BcmpPacketCfg resource_request = {
      false,
      false,
      bcmp_process_resource_discovery_request,
  };
  BcmpPacketCfg resource_reply = {
      false,
      false,
      bcmp_process_resource_discovery_reply,
  };
  BcmpPacketCfg exchange_request = {
      false,
      true,
      bcmp_process_resource_request,
  };
  BcmpPacketCfg exchange_reply = {
      true,
      false,
      NULL,
  };

  PUB_LIST.start = NULL;
  PUB_LIST.end = NULL;
  PUB_LIST.num_resources = 0;
  PUB_LIST.lock = bm_mutex_create();
  SUB_LIST.start = NULL;
  SUB_LIST.end = NULL;
  SUB_LIST.num_resources = 0;
  SUB_LIST.lock = bm_mutex_create();
  if (PUB_LIST.lock && SUB_LIST.lock) {
    err = BmOK;
  }
  bm_err_check(err, routing_init());
  bm_err_check(err,
               packet_add(&resource_request, BcmpResourceTableRequestMessage));
  bm_err_check(err, packet_add(&resource_reply, BcmpResourceTableReplyMessage));
  bm_err_check(err, packet_add(&exchange_request, BcmpResourceRequestMessage));
  bm_err_check(err, packet_add(&exchange_reply, BcmpResourceReplyMessage));

  return err;
}

/*!
  @brief Add a resource to the resource discovery module

  @details Note that you can add this for a topic you intend to publish data
           to ahead of actually publishing the data

  @param *res - resource name
  @param resource_len - length of the resource name
  @param type - publishers or subscribers
  @param timeoutMs - how long to wait to add resource in milliseconds.

  @return BmOK on success
  @return BmErr otherwise
*/
BmErr bcmp_resource_discovery_add_resource(const char *res,
                                           const uint16_t resource_len,
                                           ResourceType type,
                                           uint32_t timeoutMs) {
  BmErr err = BmETIMEDOUT;
  BcmpResourceList *res_list = (type == SUB) ? &SUB_LIST : &PUB_LIST;
  if (bm_semaphore_take(res_list->lock, timeoutMs) == BmOK) {
    // Check for resource
    if (bcmp_resource_discovery_find_resource_priv(res, resource_len, type)) {
      err = BmEAGAIN;
    } else {
      // Build resouce
      size_t resource_size = sizeof(BcmpResource) + resource_len;
      uint8_t *resource_buffer = (uint8_t *)bm_malloc(resource_size);
      BcmpResource *resource = (BcmpResource *)resource_buffer;
      resource->resource_len = resource_len;
      memcpy(resource->resource, res, resource_len);

      // Build Node
      BcmpResourceNode *resource_node =
          (BcmpResourceNode *)bm_malloc(sizeof(BcmpResourceNode));
      if (resource_node) {
        resource_node->resource = resource;
        resource_node->next = NULL;
        // Add node to list
        if (res_list->start == NULL) { // First resource
          res_list->start = resource_node;
        } else { // 2nd+ resource
          res_list->end->next = resource_node;
        }
        res_list->end = resource_node;
        res_list->num_resources++;
        err = BmOK;
      } else {
        err = BmENOMEM;
      }
    }
    bm_semaphore_give(res_list->lock);
  }
  return err;
}

/*!
  @brief Get number of resources in the table.

  @param out &num_resources - number of resources in the table.
  @param in type - publishers or subscribers
  @param in timeoutMs - how long to wait to add resource in milliseconds.

  @return true on success
  @return false otherwise
*/
BmErr bcmp_resource_discovery_get_num_resources(uint16_t *num_resources,
                                                ResourceType type,
                                                uint32_t timeoutMs) {
  BmErr err = BmETIMEDOUT;
  BcmpResourceList *res_list = (type == SUB) ? &SUB_LIST : &PUB_LIST;
  if (bm_semaphore_take(res_list->lock, timeoutMs) == BmOK) {
    *num_resources = res_list->num_resources;
    err = BmOK;
    bm_semaphore_give(res_list->lock);
  }
  return err;
}

/*!
  @brief Check if a given resource is in the table.

  @param in *res - resource name
  @param in resource_len - length of the resource name
  @param out &found - whether or not the resource was found in the table
  @param in type - publishers or subscribers
  @param in timeoutMs - how long to wait to add resource in milliseconds.

  @return true on success
  @return false otherwise
*/
BmErr bcmp_resource_discovery_find_resource(const char *res,
                                            const uint16_t resource_len,
                                            bool *found, ResourceType type,
                                            uint32_t timeoutMs) {
  BmErr err = BmETIMEDOUT;
  BcmpResourceList *res_list = (type == SUB) ? &SUB_LIST : &PUB_LIST;
  if (bm_semaphore_take(res_list->lock, timeoutMs) == BmOK) {
    *found =
        bcmp_resource_discovery_find_resource_priv(res, resource_len, type);
    err = BmOK;
    bm_semaphore_give(res_list->lock);
  }
  return err;
}

/*!
  @brief Send a bcmp resource discovery request to a node.

  @param in target_node_id - requested node id

  @return BmOK on success
  @return BmErr otherwise
*/
BmErr bcmp_resource_discovery_send_request(uint64_t target_node_id,
                                           void (*fp)(void *)) {
  BmErr err = BmEBADMSG;
  LLItem *item = NULL;
  ResourceCb cb = {fp};
  BcmpResourceTableRequest req = {
      .target_node_id = target_node_id,
  };
  err = bcmp_tx(&multicast_ll_addr, BcmpResourceTableRequestMessage,
                (uint8_t *)&req, sizeof(req), 0, NULL);
  if (err == BmOK) {
    item = ll_create_item(item, &cb, sizeof(cb), target_node_id);
    if (item && ll_item_add(&RESOURCE_REQUEST_LIST, item) == BmOK) {
      err = BmOK;
    } else {
      err = BmENOMEM;
    }
  } else {
    bm_debug("Failed to send bcmp resource table request, error %d\n", err);
  }
  return err;
}

/*!
  @brief Print the resources in the table.
*/
void bcmp_resource_discovery_print_resources(void) {
  bm_debug("Resource table:\n");
  if (PUB_LIST.num_resources &&
      bm_semaphore_take(PUB_LIST.lock, default_resource_add_timeout_ms) ==
          BmOK) {
    bm_debug("\tPubs:\n");
    uint16_t num_items = PUB_LIST.num_resources;
    BcmpResourceNode *cur_resource_node = PUB_LIST.start;
    while (num_items && cur_resource_node) {
      bm_debug("\t* %.*s\n", cur_resource_node->resource->resource_len,
               cur_resource_node->resource->resource);
      cur_resource_node = cur_resource_node->next;
      num_items--;
    }
    bm_semaphore_give(PUB_LIST.lock);
  }
  if (SUB_LIST.num_resources &&
      bm_semaphore_take(SUB_LIST.lock, default_resource_add_timeout_ms) ==
          BmOK) {
    bm_debug("\tSubs:\n");
    uint16_t num_items = SUB_LIST.num_resources;
    BcmpResourceNode *cur_resource_node = SUB_LIST.start;
    while (num_items && cur_resource_node) {
      bm_debug("\t* %.*s\n", cur_resource_node->resource->resource_len,
               cur_resource_node->resource->resource);
      cur_resource_node = cur_resource_node->next;
      num_items--;
    }
    bm_semaphore_give(SUB_LIST.lock);
  }
}

/*!
  @brief Get the resources in the table.

  @return pointer to the resource table reply, caller is responsible for freeing the memory.
*/
BcmpResourceTableReply *bcmp_resource_discovery_get_local_resources(void) {
  bool success = false;
  BcmpResourceTableReply *reply_rval = NULL;
  size_t msg_len = sizeof(BcmpResourceTableReply);
  do {
    if (!bcmp_resource_compute_list_size(PUB, &msg_len)) {
      bm_debug("Failed to get publishers list\n.");
      break;
    }
    if (!bcmp_resource_compute_list_size(SUB, &msg_len)) {
      bm_debug("Failed to get subscribers list\n.");
      break;
    }
    // Create and fill the reply
    reply_rval = (BcmpResourceTableReply *)bm_malloc(msg_len);
    if (reply_rval) {
      reply_rval->node_id = node_id();
      uint32_t data_offset = 0;
      if (!bcmp_resource_populate_msg_data(PUB, reply_rval, &data_offset)) {
        bm_debug("Failed to populate publishers list\n.");
        break;
      }
      if (!bcmp_resource_populate_msg_data(SUB, reply_rval, &data_offset)) {
        bm_debug("Failed to populate publishers list\n.");
        break;
      }
      success = true;
    }
  } while (0);

  if (!success) {
    if (reply_rval != NULL) {
      bm_free(reply_rval);
      reply_rval = NULL;
    }
  }
  return reply_rval;
}

/*!
 @brief Send a request to share a resource of interest on the network

 @details Will send sequenced packets to each available port on the device.

 @param info Resource information to share with neighbor

 @return BmOK on success
         BmErr upon failure
 */
BmErr bcmp_resource_send_request(const ResourceInfo *info) {
  if (!info) {
    return BmEINVAL;
  }

  uint16_t info_size = sizeof(ResourceInfo) + info->length;

  uint8_t num_ports = bm_l2_get_port_count();
  BcmpTxCtx ctx = {
      .dst = &multicast_ll_addr,
      .type = BcmpResourceRequestMessage,
      .data = (uint8_t *)info,
      .size = info_size,
      .seq_num = 0,
      .reply_cb = resource_discovery_reply_cb,
      .egress_port = 1,
  };

  for (; ctx.egress_port <= num_ports; ctx.egress_port++) {
    BmErr tx_err = bcmp_tx_port(ctx);
    if (tx_err != BmOK) {
      bm_debug("Failed to send bcmp resource request on port %u, error %d\n",
               ctx.egress_port, tx_err);
    }
  }

  return BmOK;
}
