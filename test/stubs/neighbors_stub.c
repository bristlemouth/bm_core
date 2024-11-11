#include "mock_bm_os.h"
#include "mock_neighbors.h"

DEFINE_FAKE_VALUE_FUNC(BmErr, bcmp_neighbor_init);
DEFINE_FAKE_VALUE_FUNC(BcmpNeighbor *, bcmp_get_neighbors, uint8_t *);
DEFINE_FAKE_VOID_FUNC(bcmp_check_neighbors);
DEFINE_FAKE_VOID_FUNC(bcmp_print_neighbor_info, BcmpNeighbor *);
DEFINE_FAKE_VALUE_FUNC(bool, bcmp_remove_neighbor_from_table, BcmpNeighbor *);
bool bcmp_free_neighbor(BcmpNeighbor *neighbor) {
  bool rval = false;
  if (neighbor) {
    if (neighbor->version_str) {
      bm_free(neighbor->version_str);
    }

    if (neighbor->device_name) {
      bm_free(neighbor->device_name);
    }

    bm_free(neighbor);
    rval = true;
  }

  return rval;
}
DEFINE_FAKE_VALUE_FUNC(BcmpNeighbor *, bcmp_find_neighbor, uint64_t);
DEFINE_FAKE_VALUE_FUNC(BcmpNeighbor *, bcmp_update_neighbor, uint64_t, uint8_t);
DEFINE_FAKE_VOID_FUNC(bcmp_neighbor_foreach, NeighborCallback);
DEFINE_FAKE_VOID_FUNC(bcmp_neighbor_register_discovery_callback,
                      NeighborDiscoveryCallback);
DEFINE_FAKE_VOID_FUNC(bcmp_neighbor_invoke_discovery_cb, bool, BcmpNeighbor *);
DEFINE_FAKE_VALUE_FUNC(BmErr, bcmp_request_neighbor_table, uint64_t,
                       const void *, NeighborRequestCallback, BmTimerCallback);
DEFINE_FAKE_VOID_FUNC(assemble_neighbor_info_list, BcmpNeighborInfo *,
                      BcmpNeighbor *, uint8_t);
