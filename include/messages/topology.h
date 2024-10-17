#pragma once

#include "messages.h"
#include "util.h"

typedef struct NeighborTableEntry {
  struct NeighborTableEntry *prevNode;
  struct NeighborTableEntry *nextNode;

  // increment this each time a port's path has been explored
  // when this is equal to the number of ports we can say
  // we have explored each ports path and can return to
  // the previous table entry to check its ports
  uint8_t ports_explored;

  bool is_root;

  BcmpNeighborTableReply *neighbor_table_reply;

} NeighborTableEntry;

typedef struct {
  NeighborTableEntry *front;
  NeighborTableEntry *back;
  NeighborTableEntry *cursor;
  uint8_t length;
  int16_t index;
} networkTopology_t;

typedef enum {
  TOPO_ASSEMBLED = 0,
  TOPO_LOADING,
  TOPO_EMPTY,
} networkTopology_status_t;

typedef void (*BcmpTopoCb)(networkTopology_t *network_topology);

void network_topology_print(networkTopology_t *network_topology);

// Neighbor table request defines, used to create the network topology
BmErr bcmp_request_neighbor_table(uint64_t target_node_id, const void *addr);

BmErr bcmp_topology_init(void);
// Topology task defines
BmErr bcmp_topology_start(const BcmpTopoCb callback);
