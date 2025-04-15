#include "messages/neighbors.h"
#include "bcmp.h"
#include "bm_config.h"
#include "bm_os.h"
#include "device.h"
#include "l2.h"
#include "ll.h"
#include "messages/info.h"
#include "packet.h"
#include <inttypes.h>
#include <string.h>

// Timer to stop waiting for a nodes neighbor table
#define bcmp_neighbor_timer_timeout_s 1
#define bcmp_table_max_len 2048

// Pointer to neighbor linked-list
static uint8_t NUM_NEIGHBORS = 0;
static NeighborDiscoveryCallback NEIGHBOR_DISCOVERY_CB = NULL;
static NeighborRequestCallback NEIGHBOR_REQUEST_CB = NULL;
static uint64_t TARGET_NODE_ID = 0;
static uint8_t NUM_PORTS = 0;
static BmTimer NEIGHBOR_TIMER = NULL;
static LL NEIGHBOR_LL = {0};
static uint32_t NEIGHBOR_ID = 0;

/*!
  @brief Send reply to neighbor table request

  @param *neighbor_table_reply - reply message
  @param *addr - ip address to send reply to
  @ret ERR_OK if successful
*/
static BmErr bcmp_send_neighbor_table(void *addr) {
  BmErr err = BmENOMEM;

  // Check our neighbors
  uint8_t num_neighbors = bcmp_get_neighbors_count();
  uint16_t neighbor_table_len = sizeof(BcmpNeighborTableReply) +
                                sizeof(BcmpPortInfo) * NUM_PORTS +
                                sizeof(BcmpNeighborInfo) * num_neighbors;

  // TODO - handle more gracefully
  if (neighbor_table_len > bcmp_table_max_len) {
    return BmEINVAL;
  }

  uint8_t *neighbor_table_reply_buff = (uint8_t *)bm_malloc(neighbor_table_len);
  if (neighbor_table_reply_buff) {
    memset(neighbor_table_reply_buff, 0, neighbor_table_len);

    BcmpNeighborTableReply *neighbor_table_reply =
        (BcmpNeighborTableReply *)neighbor_table_reply_buff;
    neighbor_table_reply->node_id = node_id();

    // set the other vars
    neighbor_table_reply->port_len = NUM_PORTS;
    neighbor_table_reply->neighbor_len = num_neighbors;

    // assemble the port list here
    for (uint8_t port = 0; port < NUM_PORTS; port++) {
      neighbor_table_reply->port_list[port].state = bm_l2_get_port_state(port);
    }

    assemble_neighbor_info_list(
        (BcmpNeighborInfo *)&neighbor_table_reply->port_list[NUM_PORTS]);

    err = bcmp_tx(addr, BcmpNeighborTableReplyMessage,
                  (uint8_t *)neighbor_table_reply, neighbor_table_len, 0, NULL);

    bm_free(neighbor_table_reply_buff);
  }

  return err;
}

/*!
  @brief Handle neighbor table requests

  @param *neighbor_table_req - message to process
  @param *src - source ip of requester
  @param *dst - destination ip of request (used for responding to the correct multicast address)
  @return BmOK if successful
  @return BmErr if failed
*/
static BmErr bcmp_process_neighbor_table_request(BcmpProcessData data) {
  BmErr err = BmENOTINTREC;
  BcmpNeighborTableRequest *request = (BcmpNeighborTableRequest *)data.payload;

  if (request->target_node_id == 0 || node_id() == request->target_node_id) {
    err = bcmp_send_neighbor_table(data.dst);
  }

  return err;
}

/*!
  @brief Handle neighbor table replies

  @param *neighbor_table_reply - reply message to process
*/
static BmErr bcmp_process_neighbor_table_reply(BcmpProcessData data) {
  BmErr err = BmENOTINTREC;
  BcmpNeighborTableReply *reply = (BcmpNeighborTableReply *)data.payload;

  if (TARGET_NODE_ID == reply->node_id) {
    err = bm_timer_stop(NEIGHBOR_TIMER, 10);
    if (NEIGHBOR_REQUEST_CB) {
      bm_err_check(err, NEIGHBOR_REQUEST_CB(reply));
      NEIGHBOR_REQUEST_CB = NULL;
    }
  }

  return err;
}

/*!
  @brief Initialize BCMP Topology Module

  @return BmOK on success
  @return BmErr on failure
*/
BmErr bcmp_neighbor_init(uint8_t num_ports) {
  BmErr err = BmOK;
  NUM_PORTS = num_ports;
  BcmpPacketCfg neighbor_request = {
      false,
      false,
      bcmp_process_neighbor_table_request,
  };
  BcmpPacketCfg neighbor_reply = {
      false,
      false,
      bcmp_process_neighbor_table_reply,
  };

  bm_err_check(err,
               packet_add(&neighbor_request, BcmpNeighborTableRequestMessage));
  bm_err_check(err, packet_add(&neighbor_reply, BcmpNeighborTableReplyMessage));

  return err;
}

/*!
  @brief Obtain neighbor count

  @return - Number of neighbors
*/
uint8_t bcmp_get_neighbors_count(void) { return NUM_NEIGHBORS; }

static BcmpNeighbor *NEIGHBOR_FOUND = NULL;
static BmErr find_node_id_traverse(void *data, void *arg) {
  BmErr ret = BmOK;
  BcmpNeighbor *neighbor = (BcmpNeighbor *)data;
  uint64_t node_id = *(uint64_t *)arg;

  if (neighbor->node_id == node_id) {
    NEIGHBOR_FOUND = neighbor;
    ret = BmEALREADY;
  }

  return ret;
}

/*!
  @bried Find neighbor entry in neighbor table

  @param node_id - neighbor's node_id

  @return pointer to neighbor if successful
  @return NULL if unsuccessful
*/
BcmpNeighbor *bcmp_find_neighbor(uint64_t node_id) {
  NEIGHBOR_FOUND = NULL;

  if (ll_traverse(&NEIGHBOR_LL, find_node_id_traverse, &node_id) !=
      BmEALREADY) {
    NEIGHBOR_FOUND = NULL;
  }

  return NEIGHBOR_FOUND;
}

static BmErr for_each_traverse(void *data, void *arg) {
  BcmpNeighbor *neighbor = (BcmpNeighbor *)data;
  NeighborCallback cb = (NeighborCallback)arg;

  if (neighbor == NEIGHBOR_LL.head->data) {
    NUM_NEIGHBORS = 0;
  }

  if (cb) {
    cb(neighbor);
  }
  NUM_NEIGHBORS++;

  return BmOK;
}

/*!
  @brief Iterate through all neighbors and call callback function for each

  @param *callback - callback function to call for each neighbor
*/
void bcmp_neighbor_foreach(NeighborCallback cb) {
  ll_traverse(&NEIGHBOR_LL, for_each_traverse, cb);
}

/*!
  @brief Check If Neighbor Is Offline

  @details Neighbor will be online until a heartbeat is not seen for 2
           heartbeat periods or longer

  @param neighbor
 */
static void neighbor_check(BcmpNeighbor *neighbor) {
  if (neighbor->online &&
      !time_remaining(
          neighbor->last_heartbeat_ticks, bm_get_tick_count(),
          bm_ms_to_ticks(2 * neighbor->heartbeat_period_s * 1000))) {
    bm_debug("🏚  Neighbor offline :'( %016" PRIx64 "\n", neighbor->node_id);
    bcmp_neighbor_invoke_discovery_cb(false, neighbor);
    neighbor->online = false;
  }
}

/*!
  @brief Check neighbor livelyness status for all neighbors
*/
void bcmp_check_neighbors(void) { bcmp_neighbor_foreach(neighbor_check); }

static BcmpNeighbor *REMOVE_NEIGHBOR = NULL;
static BmErr neighbor_port_populated_traverse(void *data, void *arg) {
  BmErr ret = BmOK;
  BcmpNeighbor *neighbor = (BcmpNeighbor *)data;
  uint8_t port = *(uint8_t *)arg;

  if (neighbor->port == port) {
    REMOVE_NEIGHBOR = neighbor;
    ret = BmEALREADY;
  }

  return ret;
}

/*!
  @brief Add neighbor to neighbor table

  @param node_id - neighbor's node_id
  @param port - BM port mask
  @return pointer to neighbor if successful, NULL otherwise (if neighbor is already present, for example)
*/
static BcmpNeighbor *bcmp_add_neighbor(uint64_t node_id, uint8_t port) {
  LLItem *item = NULL;
  BcmpNeighbor new_neighbor = {0};
  BcmpNeighbor *ret = NULL;
  new_neighbor.node_id = node_id;
  new_neighbor.port = port;
  new_neighbor.id = NEIGHBOR_ID;

  // Check to see if port is already populated, remove if so
  if (ll_traverse(&NEIGHBOR_LL, neighbor_port_populated_traverse, &port) ==
      BmEALREADY) {
    bcmp_remove_neighbor_from_table(REMOVE_NEIGHBOR);
  }

  item = ll_create_item(&new_neighbor, sizeof(BcmpNeighbor), new_neighbor.id);
  if (item && ll_item_add(&NEIGHBOR_LL, item) == BmOK) {
    NEIGHBOR_ID++;
    ret = (BcmpNeighbor *)item->data;
  }

  return ret;
}

/*!
  @brief Update neighbor information in neighbor table

  @param node_id - neighbor's node_id
  @param port - BM port mask for neighbor
  @return pointer to neighbor if successful, NULL otherwise
*/
BcmpNeighbor *bcmp_update_neighbor(uint64_t node_id, uint8_t port) {
  BcmpNeighbor *neighbor = bcmp_find_neighbor(node_id);

  if (neighbor == NULL) {
    bm_debug("🏘  Adding new neighbor! %016" PRIx64 "\n", node_id);
    neighbor = bcmp_add_neighbor(node_id, port);
    // Let's get this node's information
    bcmp_request_info(node_id, &multicast_ll_addr, NULL);
  }

  return neighbor;
}

/*!
  @brief Free neighbor data from memory. NOTE: this does NOT remove neighbor from table

  @param *neighbor - neighbor to free
  @return true if the neighbor was freed, false otherwise
*/
bool bcmp_free_neighbor(BcmpNeighbor *neighbor) {
  bool ret = false;

  if (neighbor) {
    ret = true;
    if (neighbor->version_str) {
      bm_free(neighbor->version_str);
    }

    if (neighbor->device_name) {
      bm_free(neighbor->device_name);
    }
  }

  return ret;
}

/*!
  @brief Delete neighbor from neighbor table

  @param *neighbor - pointer to neighbor to remove

  @return true if successful
  @return false otherwise
*/
bool bcmp_remove_neighbor_from_table(BcmpNeighbor *neighbor) {
  bool rval = false;
  LLItem *item = ll_get_item_by_data(&NEIGHBOR_LL, neighbor);

  if (item) {
    bcmp_free_neighbor(neighbor);
    rval = ll_remove(&NEIGHBOR_LL, neighbor->id) == BmOK;
  }

  return rval;
}

/*!
  @brief Print neighbor information to CLI

  @param *neighbor - neighbor who's information we'll print
  @return true if successful, false otherwise
*/
void bcmp_print_neighbor_info(BcmpNeighbor *neighbor) {

  if (neighbor) {
    bm_debug("Neighbor information:\n");
    bm_debug("Node ID: %016" PRIx64 "\n", neighbor->node_id);
    bm_debug("VID: %04X PID: %04X\n", neighbor->info.vendor_id,
             neighbor->info.product_id);
    bm_debug("Serial number %.*s\n", 16, neighbor->info.serial_num);
    bm_debug("GIT SHA: %" PRIX32 "\n", neighbor->info.git_sha);
    bm_debug("Version: %u.%u.%u\n", neighbor->info.ver_major,
             neighbor->info.ver_minor, neighbor->info.ver_rev);
    bm_debug("HW Version: %u\n", neighbor->info.ver_hw);
    if (neighbor->version_str) {
      bm_debug("VersionStr: %s\n", neighbor->version_str);
    }
    if (neighbor->device_name) {
      bm_debug("Device Name: %s\n", neighbor->device_name);
    }
  }
}

/*!
  @brief Register a callback to run whenever a neighbor is discovered / drops.

  @param *callback - callback function to run.
  @return none
*/
void bcmp_neighbor_register_discovery_callback(NeighborDiscoveryCallback cb) {
  NEIGHBOR_DISCOVERY_CB = cb;
}

/*!
  @brief Invoke Registered Neighbor Discovery Callback

  @param discovered has the neighbor been discovered
  @param neighbor pointer to the neighbor
*/
void bcmp_neighbor_invoke_discovery_cb(bool discovered,
                                       BcmpNeighbor *neighbor) {
  if (NEIGHBOR_DISCOVERY_CB) {
    NEIGHBOR_DISCOVERY_CB(discovered, neighbor);
  }
}

/*!
  @brief Send neighbor table request to node(s)

  @param target_node_id - target node id to send request to (0 for all nodes)
  @param *addr - ip address to send to send request to
  @ret ERR_OK if successful
*/
BmErr bcmp_request_neighbor_table(uint64_t target_node_id, const void *addr,
                                  NeighborRequestCallback request,
                                  BmTimerCallback timeout) {
  BmErr err = BmOK;
  BcmpNeighborTableRequest neighbor_table_req = {.target_node_id =
                                                     target_node_id};
  TARGET_NODE_ID = target_node_id;
  if (NEIGHBOR_TIMER) {
    bm_timer_delete(NEIGHBOR_TIMER, 10);
  }
  NEIGHBOR_TIMER = bm_timer_create("neighbor_request_timer",
                                   bcmp_neighbor_timer_timeout_s * 1000, false,
                                   NULL, timeout);

  if (NEIGHBOR_TIMER) {
    err = bm_timer_start(NEIGHBOR_TIMER, 10);
    bm_err_check(err, bcmp_tx(addr, BcmpNeighborTableRequestMessage,
                              (uint8_t *)&neighbor_table_req,
                              sizeof(neighbor_table_req), 0, NULL));
    NEIGHBOR_REQUEST_CB = request;
  } else {
    err = BmENOMEM;
  }

  return err;
}

static BmErr assemble_neighbor_info_list_traverse(void *data, void *arg) {
  static uint16_t neighbor_count = 0;
  BcmpNeighborInfo *neighbor_info_list = (BcmpNeighborInfo *)arg;
  BcmpNeighbor *neighbor = (BcmpNeighbor *)data;

  if (neighbor == NEIGHBOR_LL.head->data) {
    neighbor_count = 0;
  }

  neighbor_info_list[neighbor_count].node_id = neighbor->node_id;
  neighbor_info_list[neighbor_count].port = neighbor->port;
  neighbor_info_list[neighbor_count].online = (uint8_t)neighbor->online;

  return BmOK;
}

// assembles the neighbor info list
void assemble_neighbor_info_list(BcmpNeighborInfo *neighbor_info_list) {
  ll_traverse(&NEIGHBOR_LL, assemble_neighbor_info_list_traverse,
              neighbor_info_list);
}
