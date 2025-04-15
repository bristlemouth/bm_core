#include "messages/resource_discovery.h"
#include "bcmp.h"
#include "bm_config.h"
#include "bm_os.h"
#include "device.h"
#include "ll.h"
#include "packet.h"
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

  PUB_LIST.start = NULL;
  PUB_LIST.end = NULL;
  PUB_LIST.num_resources = 0;
  PUB_LIST.lock = bm_semaphore_create();
  SUB_LIST.start = NULL;
  SUB_LIST.end = NULL;
  SUB_LIST.num_resources = 0;
  SUB_LIST.lock = bm_semaphore_create();
  if (PUB_LIST.lock && SUB_LIST.lock) {
    err = BmOK;
  }
  bm_err_check(err,
               packet_add(&resource_request, BcmpResourceTableRequestMessage));
  bm_err_check(err, packet_add(&resource_reply, BcmpResourceTableReplyMessage));
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
    item = ll_create_item(&cb, sizeof(cb), target_node_id);
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
