#include "mock_neighbors.h"

DEFINE_FAKE_VOID_FUNC(bcmp_check_neighbors);
DEFINE_FAKE_VOID_FUNC(bcmp_print_neighbor_info, BcmpNeighbor *);
DEFINE_FAKE_VALUE_FUNC(bool, bcmp_remove_neighbor_from_table, BcmpNeighbor *);
DEFINE_FAKE_VALUE_FUNC(bool, bcmp_free_neighbor, BcmpNeighbor *);
DEFINE_FAKE_VALUE_FUNC(BcmpNeighbor *, bcmp_find_neighbor, uint64_t);
DEFINE_FAKE_VALUE_FUNC(BcmpNeighbor *, bcmp_update_neighbor, uint64_t, uint8_t);
DEFINE_FAKE_VOID_FUNC(bcmp_neighbor_foreach, NeighborCallback);
DEFINE_FAKE_VOID_FUNC(bcmp_neighbor_register_discovery_callback,
                      NeighborDiscoveryCallback);
DEFINE_FAKE_VOID_FUNC(bcmp_neighbor_invoke_discovery_cb, bool, BcmpNeighbor *);
