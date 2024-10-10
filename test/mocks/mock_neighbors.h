#include "fff.h"
#include "messages/neighbors.h"
#include "util.h"

DECLARE_FAKE_VOID_FUNC(bcmp_check_neighbors);
DECLARE_FAKE_VOID_FUNC(bcmp_print_neighbor_info, BcmpNeighbor *);
DECLARE_FAKE_VALUE_FUNC(bool, bcmp_remove_neighbor_from_table, BcmpNeighbor *);
DECLARE_FAKE_VALUE_FUNC(bool, bcmp_free_neighbor, BcmpNeighbor *);
DECLARE_FAKE_VALUE_FUNC(BcmpNeighbor *, bcmp_find_neighbor, uint64_t);
DECLARE_FAKE_VALUE_FUNC(BcmpNeighbor *, bcmp_update_neighbor, uint64_t,
                        uint8_t);
DECLARE_FAKE_VOID_FUNC(bcmp_neighbor_foreach, NeighborCallback);
DECLARE_FAKE_VOID_FUNC(bcmp_neighbor_register_discovery_callback,
                       NeighborDiscoveryCallback);
DECLARE_FAKE_VOID_FUNC(bcmp_neighbor_invoke_discovery_cb, bool, BcmpNeighbor *);
