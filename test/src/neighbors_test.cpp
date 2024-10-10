#include <gtest/gtest.h>
#include <helpers.hpp>

#include "fff.h"

DEFINE_FFF_GLOBALS;

extern "C" {
#include "messages/neighbors.h"
#include "mock_bm_os.h"
#include "mock_info.h"
}

static uint32_t CALL_COUNT = 0;

class neighbors_test : public ::testing::Test {
public:
  rnd_gen RND;

private:
protected:
  neighbors_test() {}
  ~neighbors_test() override {}
  void SetUp() override { CALL_COUNT = 0; }
  void TearDown() override {}
  static void neighbor_cb_tester(bool discovered, BcmpNeighbor *neighbor) {
    (void)discovered;
    (void)neighbor;
    CALL_COUNT++;
  }
};

TEST_F(neighbors_test, update_neighbor) {
  uint64_t node_id = (uint64_t)RND.rnd_int(UINT64_MAX, UINT8_MAX);
  uint8_t neighbor_count = (uint8_t)RND.rnd_int(UINT8_MAX, 128);
  uint8_t num_neighbors = 0;
  char *version_string = (char *)malloc(RND.rnd_int(UINT8_MAX, 8));
  char *device_name = (char *)malloc(RND.rnd_int(UINT8_MAX, 8));
  BcmpNeighbor *neighbors[neighbor_count];

  bcmp_print_neighbor_info(NULL);

  // Test a single add
  BcmpNeighbor *neighbor = bcmp_update_neighbor(node_id, 1);
  neighbor->version_str = version_string;
  neighbor->device_name = device_name;
  ASSERT_NE(neighbor, nullptr);
  ASSERT_EQ(bcmp_request_info_fake.call_count, 1);
  ASSERT_EQ(bcmp_update_neighbor(node_id, 1), neighbor);
  RESET_FAKE(bcmp_request_info);
  bcmp_print_neighbor_info(neighbor);
  ASSERT_EQ(bcmp_remove_neighbor_from_table(neighbor), true);

  uint32_t seed = time(NULL);
  rand_sequence_unique rsu(seed, seed + 1);

  // Test multiple adds
  for (uint8_t i = 0; i < neighbor_count; i++) {
    neighbors[i] = bcmp_update_neighbor(rsu.next(), 1);
    neighbors[i]->online = true;
    neighbors[i]->last_heartbeat_ticks = 0;
    ASSERT_NE(neighbors[i], nullptr);
  }

  bcmp_check_neighbors();
  bm_get_tick_count_fake.return_val = 100000;
  neighbors[0]->online = false;
  bcmp_check_neighbors();
  ASSERT_EQ(bcmp_get_neighbors(&num_neighbors), neighbors[0]);
  ASSERT_EQ(num_neighbors, neighbor_count);

  for (int16_t i = neighbor_count - 1; i >= 0; i--) {
    ASSERT_EQ(bcmp_remove_neighbor_from_table(neighbors[i]), true);
  }
  RESET_FAKE(bcmp_request_info);
}

TEST_F(neighbors_test, neighbor_cb) {
  bcmp_neighbor_register_discovery_callback(neighbor_cb_tester);
  bcmp_neighbor_invoke_discovery_cb(true, NULL);
  ASSERT_EQ(CALL_COUNT, 1);
}
