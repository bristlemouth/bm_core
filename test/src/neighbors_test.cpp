#include <gtest/gtest.h>
#include <helpers.hpp>

#include "fff.h"

DEFINE_FFF_GLOBALS;

extern "C" {
#include "messages/neighbors.h"
#include "mock_bcmp.h"
#include "mock_bm_os.h"
#include "mock_device.h"
#include "mock_info.h"
#include "mock_packet.h"
}

static uint32_t CALL_COUNT = 0;

class Neighbors : public ::testing::Test {
public:
  rnd_gen RND;

private:
protected:
  uint32_t size_info;
  Neighbors() {}
  ~Neighbors() override {}
  void SetUp() override { CALL_COUNT = 0; }
  void TearDown() override {}
  static void neighbor_cb_tester(bool discovered, BcmpNeighbor *neighbor) {
    (void)discovered;
    (void)neighbor;
    CALL_COUNT++;
  }
  static void neighbor_timer_handler(BmTimer tmr) { (void)tmr; }
  static BmErr neighbor_request_cb(BcmpNeighborTableReply *reply) {
    (void)reply;
    return BmOK;
  }
};

TEST_F(Neighbors, update_neighbor) {
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
    neighbors[i] = bcmp_update_neighbor(rsu.next(), i + 2);
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

TEST_F(Neighbors, neighbor_cb) {
  bcmp_neighbor_register_discovery_callback(neighbor_cb_tester);
  bcmp_neighbor_invoke_discovery_cb(true, NULL);
  ASSERT_EQ(CALL_COUNT, 1);
}

/*!
  @brief Test obtaining a neighbor table with a request call and processing
         the reply callback
*/
TEST_F(Neighbors, request_table) {
  uint32_t addr = (uint32_t)RND.rnd_int(UINT32_MAX, UINT16_MAX);
  uint64_t node_id = (uint64_t)RND.rnd_int(UINT32_MAX, UINT16_MAX);
  BcmpNeighborTableReply *reply;
  BcmpProcessData data;
  reply = (BcmpNeighborTableReply *)bm_malloc(sizeof(BcmpNeighborTableReply));
  reply->node_id = node_id;
  reply->port_len = 2;
  reply->neighbor_len = 2;
  data.payload = (uint8_t *)reply;

  EXPECT_EQ(bcmp_neighbor_init(2), BmOK);

  bm_timer_start_fake.return_val = BmOK;
  bcmp_tx_fake.return_val = BmOK;
  bm_timer_create_fake.return_val =
      (BmTimer)RND.rnd_int(UINT32_MAX, UINT16_MAX);
  EXPECT_EQ(bcmp_request_neighbor_table(node_id, &addr, neighbor_request_cb,
                                        neighbor_timer_handler),
            BmOK);

  // Variable SENT_REQUEST currently un-reachable, cannot test full function
  packet_process_invoke(BcmpNeighborTableReplyMessage, data);

  // Test failures
  bcmp_tx_fake.return_val = BmEBADMSG;
  EXPECT_NE(bcmp_request_neighbor_table(node_id, &addr, NULL, NULL), BmOK);
  bm_timer_start_fake.return_val = BmENOMEM;
  EXPECT_NE(bcmp_request_neighbor_table(node_id, &addr, NULL, NULL), BmOK);
  RESET_FAKE(bcmp_tx);

  bm_free(reply);
  packet_cleanup();
}

/*!
 @brief Test request message callback
*/
TEST_F(Neighbors, request_reply) {
  BcmpNeighborTableRequest request = {
      (uint64_t)RND.rnd_int(UINT32_MAX, UINT16_MAX),
  };
  BcmpProcessData data;
  data.payload = (uint8_t *)&request;

  EXPECT_EQ(bcmp_neighbor_init(2), BmOK);

  node_id_fake.return_val = request.target_node_id;
  bcmp_tx_fake.return_val = BmOK;
  EXPECT_EQ(packet_process_invoke(BcmpNeighborTableRequestMessage, data), BmOK);

  request.target_node_id = 0;
  EXPECT_EQ(packet_process_invoke(BcmpNeighborTableRequestMessage, data), BmOK);

  // Test failures
  bcmp_tx_fake.return_val = BmEBADMSG;
  EXPECT_NE(packet_process_invoke(BcmpNeighborTableRequestMessage, data), BmOK);
  node_id_fake.return_val = 1;
  EXPECT_NE(packet_process_invoke(BcmpNeighborTableRequestMessage, data), BmOK);

  packet_cleanup();
}

TEST_F(Neighbors, same_port_node_add) {
  uint64_t node_id_init_1 = (uint64_t)RND.rnd_int(UINT64_MAX, UINT8_MAX);
  uint64_t node_id_init_2 = (uint64_t)RND.rnd_int(UINT64_MAX, UINT8_MAX);
  uint64_t node_id_init_3 = (uint64_t)RND.rnd_int(UINT64_MAX, UINT8_MAX);
  uint8_t test_num = (uint8_t)RND.rnd_int(UINT8_MAX, 128);

  BcmpNeighbor *neighbor_first = bcmp_update_neighbor(node_id_init_1, 1);
  BcmpNeighbor *neighbor_second = bcmp_update_neighbor(node_id_init_2, 2);
  BcmpNeighbor *neighbor_third = bcmp_update_neighbor(node_id_init_3, 2);
  EXPECT_NE(neighbor_first, nullptr);
  EXPECT_NE(neighbor_second, nullptr);
  EXPECT_NE(neighbor_third, nullptr);
  EXPECT_EQ(bcmp_request_info_fake.call_count, 3);
  RESET_FAKE(bcmp_request_info);
  EXPECT_NE(neighbor_second, neighbor_first);

  for (uint8_t i = 0; i < test_num; i++) {
    uint64_t node_id_replace = (uint64_t)RND.rnd_int(UINT64_MAX, UINT8_MAX);
    uint64_t node_id_compare = 0;
    uint8_t to_replace = i % 3 + 1;
    BcmpNeighbor *neighbor_replace =
        bcmp_update_neighbor(node_id_replace, to_replace);

    EXPECT_NE(neighbor_replace, nullptr);
    EXPECT_EQ(bcmp_request_info_fake.call_count, 1);
    RESET_FAKE(bcmp_request_info);

    switch (to_replace) {
    case 1:
      node_id_compare = node_id_init_1;
      break;
    case 2:
      node_id_compare = node_id_init_2;
      break;
    case 3:
      node_id_compare = node_id_init_3;
      break;
    default:
      FAIL();
      break;
    }
    EXPECT_EQ(bcmp_find_neighbor(node_id_compare), nullptr);
    EXPECT_EQ(bcmp_find_neighbor(node_id_replace), neighbor_replace);
  }
}
