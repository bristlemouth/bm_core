#include "topology.h"
#include "bcmp.h"
#include "bm_config.h"
#include "bm_os.h"
#include "device.h"
#include "l2.h"
#include "messages.h"
#include "messages/neighbors.h"
#include "packet.h"
#include "util.h"
#include <inttypes.h>
#include <string.h>

#define bcmp_topo_evt_queue_len 32

typedef enum {
  BcmpTopoEvtStart,
  BcmpTopoEvtCheckNode,
  BcmpTopoEvtAddNode,
  BcmpTopoEvtTimeout,
  BcmpTopoEvtRestart,
  BcmpTopoEvtEnd,
} BcmpTopoQueueType;

typedef struct {
  BcmpTopoQueueType type;
  NeighborTableEntry *neighbor_entry;
  BcmpTopoCb callback;
} BcmpTopoQueueItem;

typedef struct {
  BmQueue evt_queue;
  NetworkTopology *network_topology;
  bool in_progress;
  BcmpTopoCb callback;
} BcmpTopoContext;

static BcmpTopoContext CTX;
static void *BCMP_TOPOLOGY_TASK = NULL;

static bool SENT_REQUEST = false;
static bool INSERT_BEFORE = false;

static void bcmp_topology_thread(void *parameters);
static void free_neighbor_table_entry(NeighborTableEntry **entry);
static NetworkTopology *new_network_topology(void);
static void free_network_topology(NetworkTopology **network_topology);
static void network_topology_clear(NetworkTopology *network_topology);
static void network_topology_move_to_front(NetworkTopology *network_topology);
static void network_topology_move_prev(NetworkTopology *network_topology);
static void network_topology_move_next(NetworkTopology *network_topology);
static void network_topology_prepend(NetworkTopology *network_topology,
                                     NeighborTableEntry *entry);
static void network_topology_append(NetworkTopology *network_topology,
                                    NeighborTableEntry *entry);
static void network_topology_insert_after(NetworkTopology *network_topology,
                                          NeighborTableEntry *entry);
static void network_topology_insert_before(NetworkTopology *network_topology,
                                           NeighborTableEntry *entry);
static void network_topology_delete_back(NetworkTopology *network_topology);
static void
network_topology_increment_port_count(NetworkTopology *network_topology);
static uint8_t
network_topology_get_port_count(NetworkTopology *network_topology);
static bool network_topology_is_root(NetworkTopology *network_topology);
static bool
network_topology_node_id_in_topology(NetworkTopology *network_topology,
                                     uint64_t node_id);
static uint64_t
network_topology_check_neighbor_node_ids(NetworkTopology *network_topology);
static bool
network_topology_check_all_ports_explored(NetworkTopology *network_topology);

/*!
  @brief Timer handler for timeouts while waiting for neigbor tables

  @details No work is done in the timer handler, but instead an event is queued
           up to be handled in the BCMP task.

  \param tmr unused
  \return none
*/
static void topology_timer_handler(BmTimer tmr) {
  (void)tmr;

  BcmpTopoQueueItem item = {BcmpTopoEvtTimeout, NULL, NULL};

  bm_queue_send(CTX.evt_queue, &item, 0);
}

static BmErr neighbor_request_cb(BcmpNeighborTableReply *reply) {
  BmErr err = BmENOMEM;

  if (SENT_REQUEST) {
    // here we will assemble the entry for the SM
    // malloc the buffer then memcpy it over
    uint8_t *neighbor_entry_buff =
        (uint8_t *)bm_malloc(sizeof(NeighborTableEntry));
    if (neighbor_entry_buff) {

      memset(neighbor_entry_buff, 0, sizeof(NeighborTableEntry));
      NeighborTableEntry *neighbor_entry =
          (NeighborTableEntry *)neighbor_entry_buff;

      uint16_t neighbor_table_len =
          sizeof(BcmpNeighborTableReply) +
          sizeof(BcmpPortInfo) * reply->port_len +
          sizeof(BcmpNeighborInfo) * reply->neighbor_len;
      neighbor_entry->neighbor_table_reply =
          (BcmpNeighborTableReply *)bm_malloc(neighbor_table_len);

      memcpy(neighbor_entry->neighbor_table_reply, reply, neighbor_table_len);

      BcmpTopoQueueItem item = {.type = BcmpTopoEvtAddNode,
                                .neighbor_entry = neighbor_entry,
                                .callback = NULL};

      err = bm_queue_send(CTX.evt_queue, &item, 0);
    }
  }

  return err;
}

static void process_start_topology_event(void) {
  // here we will need to kick off the topo process by looking at our own neighbors and then sending out a request
  CTX.network_topology = new_network_topology();

  static uint8_t num_ports = 0;
  num_ports = bm_l2_get_num_ports();

  // Check our neighbors
  uint8_t num_neighbors = 0;
  BcmpNeighbor *neighbor = bcmp_get_neighbors(&num_neighbors);

  // here we will assemble the entry for the SM
  uint8_t *neighbor_entry_buff =
      (uint8_t *)bm_malloc(sizeof(NeighborTableEntry));

  if (neighbor_entry_buff) {
    memset(neighbor_entry_buff, 0, sizeof(NeighborTableEntry));

    NeighborTableEntry *neighbor_entry =
        (NeighborTableEntry *)(neighbor_entry_buff);

    uint32_t neighbor_table_len = sizeof(BcmpNeighborTableReply) +
                                  sizeof(BcmpPortInfo) * num_ports +
                                  sizeof(BcmpNeighborInfo) * num_neighbors;
    neighbor_entry->neighbor_table_reply =
        (BcmpNeighborTableReply *)bm_malloc(neighbor_table_len);

    neighbor_entry->neighbor_table_reply->node_id = node_id();

    // set the other vars
    neighbor_entry->neighbor_table_reply->port_len = num_ports;
    neighbor_entry->neighbor_table_reply->neighbor_len = num_neighbors;

    // assemble the port list here
    for (uint8_t port = 0; port < num_ports; port++) {
      neighbor_entry->neighbor_table_reply->port_list[port].state =
          bm_l2_get_port_state(port);
    }

    assemble_neighbor_info_list(
        (BcmpNeighborInfo *)&neighbor_entry->neighbor_table_reply
            ->port_list[num_ports],
        neighbor, num_neighbors);

    // this is the root
    neighbor_entry->is_root = true;

    network_topology_append(CTX.network_topology, neighbor_entry);
    network_topology_move_to_front(CTX.network_topology);

    BcmpTopoQueueItem check_item = {BcmpTopoEvtCheckNode, NULL, NULL};
    bm_queue_send(CTX.evt_queue, &check_item, 0);
  }
}

static void process_check_node_event(void) {
  if (network_topology_check_all_ports_explored(CTX.network_topology)) {
    if (network_topology_is_root(CTX.network_topology)) {
      BcmpTopoQueueItem end_item = {BcmpTopoEvtEnd, NULL, NULL};
      bm_queue_send(CTX.evt_queue, &end_item, 0);
    } else {
      if (INSERT_BEFORE) {
        network_topology_move_next(CTX.network_topology);
      } else {
        network_topology_move_prev(CTX.network_topology);
      }
      BcmpTopoQueueItem check_item = {BcmpTopoEvtCheckNode, NULL, NULL};
      bm_queue_send(CTX.evt_queue, &check_item, 0);
    }
  } else {
    if (network_topology_is_root(CTX.network_topology) &&
        network_topology_get_port_count(CTX.network_topology) == 1) {
      INSERT_BEFORE = true;
    }
    uint64_t new_node =
        network_topology_check_neighbor_node_ids(CTX.network_topology);
    network_topology_increment_port_count(CTX.network_topology);
    if (new_node) {
      bcmp_request_neighbor_table(new_node, &multicast_global_addr,
                                  neighbor_request_cb, topology_timer_handler);
    } else {
      if (INSERT_BEFORE) {
        network_topology_move_next(CTX.network_topology);
      } else {
        network_topology_move_prev(CTX.network_topology);
      }
      BcmpTopoQueueItem check_item = {BcmpTopoEvtCheckNode, NULL, NULL};
      bm_queue_send(CTX.evt_queue, &check_item, 0);
    }
  }
}

/*!
  @brief BCMP Topology Task

  BmErr Handles stepping through the network to request node's neighbor
           tables. Callback can be assigned/called to do something with the assembled network
           topology.

  @param parameters unused
  @return none
*/
static void bcmp_topology_thread(void *parameters) {
  (void)parameters;

  for (;;) {
    BcmpTopoQueueItem item;

    bm_queue_receive(CTX.evt_queue, &item, UINT32_MAX);

    switch (item.type) {
    case BcmpTopoEvtStart: {
      if (!CTX.in_progress) {
        CTX.in_progress = true;
        SENT_REQUEST = true;
        CTX.callback = item.callback;
        process_start_topology_event();
      } else {
        item.callback(NULL);
      }
      break;
    }

    case BcmpTopoEvtAddNode: {
      if (item.neighbor_entry) {
        if (!network_topology_node_id_in_topology(
                CTX.network_topology,
                item.neighbor_entry->neighbor_table_reply->node_id)) {
          if (INSERT_BEFORE) {
            network_topology_insert_before(CTX.network_topology,
                                           item.neighbor_entry);
            network_topology_move_prev(CTX.network_topology);
          } else {
            network_topology_insert_after(CTX.network_topology,
                                          item.neighbor_entry);
            network_topology_move_next(CTX.network_topology);
          }
          network_topology_increment_port_count(
              CTX.network_topology); // we have come from one of the ports so it must have been checked
        }
      }
      BcmpTopoQueueItem check_item = {BcmpTopoEvtCheckNode, NULL, NULL};
      bm_queue_send(CTX.evt_queue, &check_item, 0);
      break;
    }

    case BcmpTopoEvtCheckNode: {
      process_check_node_event();
      break;
    }

    case BcmpTopoEvtEnd: {
      if (CTX.callback) {
        CTX.callback(CTX.network_topology);
      }
      // Free the network topology now that we are done
      free_network_topology(&CTX.network_topology);
      INSERT_BEFORE = false;
      SENT_REQUEST = false;
      CTX.in_progress = false;
      break;
    }

    case BcmpTopoEvtTimeout: {
      if (INSERT_BEFORE) {
        network_topology_move_next(CTX.network_topology);
      } else {
        network_topology_move_prev(CTX.network_topology);
      }
      BcmpTopoQueueItem check_item = {BcmpTopoEvtCheckNode, NULL, NULL};
      bm_queue_send(CTX.evt_queue, &check_item, 0);
      break;
    }

    default: {
      break;
    }
    }
  }
}

BmErr bcmp_topology_start(BcmpTopoCb callback) {
  BmErr err = BmOK;

  // create the task if it is not already created
  if (!BCMP_TOPOLOGY_TASK) {
    CTX.evt_queue =
        bm_queue_create(bcmp_topo_evt_queue_len, sizeof(BcmpTopoQueueItem));
    err = !CTX.evt_queue ? BmENOMEM : BmOK;
    bm_err_check(err,
                 bm_task_create(bcmp_topology_thread, "BCMP_TOPO", 1024, NULL,
                                bcmp_topo_task_priority, &BCMP_TOPOLOGY_TASK));
  }

  // send the first request out
  BcmpTopoQueueItem item = {BcmpTopoEvtStart, NULL, callback};
  bm_err_check(err, bm_queue_send(CTX.evt_queue, &item, 0));

  return err;
}

/*!
 @brief Free a neighbor table entry that is within the network topology

 @param entry entry to be freed
 */
static void free_neighbor_table_entry(NeighborTableEntry **entry) {

  if (entry != NULL && *entry != NULL) {
    bm_free((*entry)->neighbor_table_reply);
    bm_free(*entry);
    *entry = NULL;
  }
}
static NetworkTopology *new_network_topology(void) {
  NetworkTopology *network_topology =
      (NetworkTopology *)bm_malloc(sizeof(NetworkTopology));
  memset(network_topology, 0, sizeof(NetworkTopology));
  network_topology->index = -1;
  return network_topology;
}

static void free_network_topology(NetworkTopology **network_topology) {
  if (network_topology != NULL && *network_topology != NULL) {
    network_topology_clear(*network_topology);
    bm_free(*network_topology);
    *network_topology = NULL;
  }
}

static void network_topology_clear(NetworkTopology *network_topology) {
  if (network_topology) {
    while (network_topology->length) {
      network_topology_delete_back(network_topology);
    }
  }
}

static void network_topology_move_to_front(NetworkTopology *network_topology) {
  if (network_topology && network_topology->length) {
    network_topology->cursor = network_topology->front;
    network_topology->index = 0;
  }
}

static void network_topology_move_prev(NetworkTopology *network_topology) {
  if (network_topology) {
    if (network_topology->index > 0) {
      network_topology->cursor = network_topology->cursor->prevNode;
      network_topology->index--;
    }
  }
}

static void network_topology_move_next(NetworkTopology *network_topology) {
  if (network_topology) {
    if (network_topology->index < network_topology->length - 1) {
      network_topology->cursor = network_topology->cursor->nextNode;
      network_topology->index++;
    }
  }
}

// insert at the front
static void network_topology_prepend(NetworkTopology *network_topology,
                                     NeighborTableEntry *entry) {
  if (network_topology && entry) {
    if (network_topology->length == 0) {
      network_topology->front = entry;
      network_topology->back = entry;
      network_topology->front->prevNode = NULL;
      network_topology->front->nextNode = NULL;
      network_topology->index++;
    } else {
      entry->nextNode = network_topology->front;
      network_topology->front->prevNode = entry;
      network_topology->front = entry;
    }
    network_topology->length++;
    if (network_topology->index >= 0) {
      network_topology->index++;
    }
  }
}

// insert at the end
static void network_topology_append(NetworkTopology *network_topology,
                                    NeighborTableEntry *entry) {
  if (network_topology && entry) {
    if (network_topology->length == 0) {
      network_topology->front = entry;
      network_topology->back = entry;
      network_topology->front->prevNode = NULL;
      network_topology->front->nextNode = NULL;
      network_topology->index++;
    } else {
      entry->prevNode = network_topology->back;
      network_topology->back->nextNode = entry;
      network_topology->back = entry;
    }
    network_topology->length++;
  }
}

// insert after the cursor entry
static void network_topology_insert_after(NetworkTopology *network_topology,
                                          NeighborTableEntry *entry) {
  if (network_topology && entry) {
    if (network_topology->length == 0) {
      network_topology_append(network_topology, entry);
    } else if (network_topology->index == network_topology->length - 1) {
      network_topology_append(network_topology, entry);
    } else {
      entry->nextNode = network_topology->cursor->nextNode;
      network_topology->cursor->nextNode->prevNode = entry;
      entry->prevNode = network_topology->cursor;
      network_topology->cursor->nextNode = entry;
      network_topology->length++;
    }
  }
}

// insert before the cursor entry
static void network_topology_insert_before(NetworkTopology *network_topology,
                                           NeighborTableEntry *entry) {
  if (network_topology && entry) {
    if (network_topology->length == 0) {
      network_topology_prepend(network_topology, entry);
    } else if (network_topology->index == 0) {
      network_topology_prepend(network_topology, entry);
    } else {
      network_topology->cursor->prevNode->nextNode = entry;
      entry->nextNode = network_topology->cursor;
      entry->prevNode = network_topology->cursor->prevNode;
      network_topology->cursor->prevNode = entry;
      network_topology->length++;
      network_topology->index++;
    }
  }
}

// delete the last entry
static void network_topology_delete_back(NetworkTopology *network_topology) {
  NeighborTableEntry *temp_entry = NULL;

  if (network_topology && network_topology->length) {
    temp_entry = network_topology->back;
    if (network_topology->length > 1) {
      network_topology->back = network_topology->back->prevNode;
      network_topology->back->nextNode = NULL;
      if (network_topology->index == network_topology->length - 1) {
        network_topology->cursor = network_topology->back;
      }
    } else {
      network_topology->cursor = NULL;
      network_topology->back = NULL;
      network_topology->front = NULL;
      network_topology->index = -1;
    }
    network_topology->length--;
    free_neighbor_table_entry(&temp_entry);
  }
}

static void
network_topology_increment_port_count(NetworkTopology *network_topology) {
  if (network_topology && network_topology->cursor) {
    network_topology->cursor->ports_explored++;
  }
}

static uint8_t
network_topology_get_port_count(NetworkTopology *network_topology) {
  if (network_topology && network_topology->cursor) {
    return network_topology->cursor->ports_explored;
  }
  return -1;
}

static bool network_topology_is_root(NetworkTopology *network_topology) {
  if (network_topology && network_topology->cursor) {
    return network_topology->cursor->is_root;
  }
  return false;
}

static bool
network_topology_node_id_in_topology(NetworkTopology *network_topology,
                                     uint64_t node_id) {
  NeighborTableEntry *temp_entry;
  if (network_topology) {
    for (temp_entry = network_topology->front; temp_entry != NULL;
         temp_entry = temp_entry->nextNode) {
      if (temp_entry->neighbor_table_reply->node_id == node_id) {
        return true;
      }
    }
  }
  return false;
}

static uint64_t
network_topology_check_neighbor_node_ids(NetworkTopology *network_topology) {
  if (network_topology) {
    for (uint8_t neighbors_count = 0;
         neighbors_count <
         network_topology->cursor->neighbor_table_reply->neighbor_len;
         neighbors_count++) {
      BcmpNeighborTableReply *temp_table =
          network_topology->cursor->neighbor_table_reply;
      BcmpNeighborInfo *neighbor_info =
          (BcmpNeighborInfo *)&temp_table->port_list[temp_table->port_len];
      uint64_t node_id = neighbor_info[neighbors_count].node_id;
      if (!network_topology_node_id_in_topology(network_topology, node_id)) {
        return node_id;
      }
    }
  }
  return 0;
}

static bool
network_topology_check_all_ports_explored(NetworkTopology *network_topology) {
  if (network_topology) {
    uint16_t neighbors_online_count = 0;
    NeighborTableEntry *cursor_entry = network_topology->cursor;
    uint8_t num_ports = cursor_entry->neighbor_table_reply->port_len;
    BcmpNeighborInfo *neighbor_info =
        (BcmpNeighborInfo *)&cursor_entry->neighbor_table_reply
            ->port_list[num_ports];

    // loop through the neighbors and make sure they port is up
    for (uint16_t neighbor_count = 0;
         neighbor_count <
         network_topology->cursor->neighbor_table_reply->neighbor_len;
         neighbor_count++) {
      // get the port number that is associated with a neighbor
      uint8_t neighbors_port_number =
          neighbor_info[neighbor_count].port -
          1; // subtract 1 since we save the ports as 1,2,... (this means a port cannot be labeled '0')
      if (network_topology->cursor->neighbor_table_reply
              ->port_list[neighbors_port_number]
              .state) {
        neighbors_online_count++;
      }
    }

    if (neighbors_online_count == network_topology->cursor->ports_explored) {
      return true;
    }
  }
  return false;
}

void network_topology_print(NetworkTopology *network_topology) {
  if (network_topology) {
    NeighborTableEntry *temp_entry;
    for (temp_entry = network_topology->front; temp_entry != NULL;
         temp_entry = temp_entry->nextNode) {
      if (temp_entry->prevNode) {
        BcmpNeighborTableReply *temp_table = temp_entry->neighbor_table_reply;
        BcmpNeighborInfo *neighbor_info =
            (BcmpNeighborInfo *)&temp_table->port_list[temp_table->port_len];
        for (uint16_t neighbor_count = 0;
             neighbor_count < temp_table->neighbor_len; neighbor_count++) {
          if (temp_entry->prevNode->neighbor_table_reply->node_id ==
              neighbor_info[neighbor_count].node_id) {
            bm_debug("%u:", neighbor_info[neighbor_count].port);
          }
        }
      }
      if (temp_entry->is_root) {
        bm_debug("(root)%016" PRIx64 "",
                 temp_entry->neighbor_table_reply->node_id);
      } else {
        bm_debug("%016" PRIx64 "", temp_entry->neighbor_table_reply->node_id);
      }
      if (temp_entry->nextNode) {
        BcmpNeighborTableReply *temp_table = temp_entry->neighbor_table_reply;
        BcmpNeighborInfo *neighbor_info =
            (BcmpNeighborInfo *)&temp_table->port_list[temp_table->port_len];
        for (uint16_t neighbor_count = 0;
             neighbor_count < temp_table->neighbor_len; neighbor_count++) {
          if (temp_entry->nextNode->neighbor_table_reply->node_id ==
              neighbor_info[neighbor_count].node_id) {
            bm_debug(":%u", neighbor_info[neighbor_count].port);
          }
        }
      }
      if (temp_entry != network_topology->back &&
          temp_entry->nextNode != NULL) {
        bm_debug(" | ");
      }
    }
    bm_debug("\n");
  } else {
    bm_debug("NULL topology, thread may be busy servicing another topology "
             "request\n");
  }
}
