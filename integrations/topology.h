#pragma once

#include "messages.h"
#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

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
} NetworkTopology;

typedef void (*BcmpTopoCb)(NetworkTopology *network_topology);

void network_topology_print(NetworkTopology *network_topology);
BmErr bcmp_topology_init(void);
BmErr bcmp_topology_start(const BcmpTopoCb callback);

#ifdef __cplusplus
}
#endif
