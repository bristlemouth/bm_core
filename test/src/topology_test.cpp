#include <gtest/gtest.h>
#include <helpers.hpp>

#include "fff.h"

extern "C" {
#include "messages/topology.h"
#include "mock_bcmp.h"
#include "mock_bm_os.h"
#include "mock_device.h"
#include "mock_packet.h"
}

DEFINE_FFF_GLOBALS;

class topology_test : public ::testing::Test {
public:
  rnd_gen RND;
  static void topology_cb(networkTopology_t *topology) {}

private:
protected:
  BcmpNeighborTableReply *replies;
  NeighborTableEntry *entries;
  uint8_t size_info;
  uint8_t size_reply;
  topology_test() {}
  ~topology_test() override {}
  void SetUp() override {}
  void TearDown() override {}

  /*!
    @brief Build A Random Topology

    @details This builds a random length topology with each node having
             two ports

    @param topology pointer to topology struct
  */
  void topology_builder_helper(networkTopology_t *topology) {
    uint8_t num_nodes = (uint8_t)RND.rnd_int(UINT8_MAX, UINT8_MAX / 2);
    uint8_t num_ports = (uint8_t)RND.rnd_int(5, 2);
    size_info = num_ports * num_nodes * sizeof(BcmpPortInfo) +
                num_ports * num_nodes * sizeof(BcmpNeighborInfo);
    size_reply = sizeof(BcmpNeighborTableReply) + size_info;
    uint64_t next_node_id = 0;
    entries =
        (NeighborTableEntry *)bm_malloc(num_nodes * sizeof(NeighborTableEntry));
    replies = (BcmpNeighborTableReply *)bm_malloc(num_nodes * size_reply);
    BcmpPortInfo *info =
        (BcmpPortInfo *)bm_malloc(num_ports * sizeof(BcmpPortInfo));
    BcmpNeighborInfo *neighbors =
        (BcmpNeighborInfo *)bm_malloc(num_ports * sizeof(BcmpNeighborInfo));

    for (int16_t i = num_nodes - 1; i >= 0; i--) {
      if (i == num_nodes - 1) {
        replies[i * size_reply].node_id = (uint64_t)RND.rnd_int(UINT64_MAX, 1);
      } else {
        replies[i * size_reply].node_id = next_node_id;
      }
      next_node_id = (uint64_t)RND.rnd_int(UINT64_MAX, 1);
      replies[i * size_reply].port_len = num_ports;
      replies[i * size_reply].neighbor_len = num_ports;
      for (uint8_t j = 0; j < num_ports; j++) {
        info[j].type = 0;
        info[j].state = true;
        neighbors[j].port = j;
        if (i < num_nodes - 1 && j == 0) {
          neighbors[j].node_id = replies[(i + 1) * size_reply].node_id;
          neighbors[j].online = true;
        } else if (i < num_nodes - 1 && j == 1) {
          neighbors[j].node_id = next_node_id;
          neighbors[j].online = true;
        } else {
          neighbors[j].online = false;
        }
      }
      memcpy(replies[i * size_reply].port_list, &info,
             sizeof(BcmpPortInfo) * num_ports);
      memcpy(&replies[i * size_reply].port_list[num_ports], neighbors,
             sizeof(BcmpNeighborInfo) * num_ports);
      if (i == num_nodes - 1) {
        entries[i].is_root = false;
        topology->back = &entries[i];
        entries[i].nextNode = NULL;
        entries[i].prevNode = NULL;
      } else if (i == 0) {
        entries[i].is_root = true;
        entries[i + 1].prevNode = &entries[i];
        entries[i].nextNode = &entries[i + 1];
        entries[i].prevNode = NULL;
        topology->front = &entries[i];
      } else {
        entries[i].is_root = false;
        entries[i + 1].prevNode = &entries[i];
        entries[i].nextNode = &entries[i + 1];
      }
      entries[i].neighbor_table_reply = &replies[i * size_reply];
    }
    bm_free(info);
    bm_free(neighbors);
  }
  void topology_builder_cleanup(void) {
    bm_free(replies);
    bm_free(entries);
  }
};

/*!
  @brief Test starting a topology request
*/
TEST_F(topology_test, topology_start) {
  // Test passing conditions
  bm_task_create_fake.return_val = BmOK;
  bm_queue_create_fake.return_val = (void *)RND.rnd_int(UINT32_MAX, UINT16_MAX);
  EXPECT_EQ(bcmp_topology_start(topology_cb), BmOK);
  EXPECT_EQ(bcmp_topology_start(NULL), BmOK);

  // Test failures
  bm_task_create_fake.return_val = BmENOMEM;
  EXPECT_NE(bcmp_topology_start(topology_cb), BmOK);
  bm_queue_create_fake.return_val = NULL;
  EXPECT_NE(bcmp_topology_start(topology_cb), BmOK);

  RESET_FAKE(bm_task_create);
  RESET_FAKE(bm_queue_create);
}

/*!
  @brief Test obtaining a neighbor table with a request call and processing
         the reply callback
*/
TEST_F(topology_test, request_table) {
  uint32_t addr = (uint32_t)RND.rnd_int(UINT32_MAX, UINT16_MAX);
  uint64_t node_id = (uint64_t)RND.rnd_int(UINT32_MAX, UINT16_MAX);
  BcmpNeighborTableReply *reply;
  networkTopology_t topology;
  BcmpProcessData data;
  topology_builder_helper(&topology);
  reply = (BcmpNeighborTableReply *)bm_malloc(sizeof(BcmpNeighborTableReply) +
                                              size_info);
  reply->node_id = node_id;
  reply->port_len = 2;
  reply->neighbor_len = 2;
  memcpy(reply->port_list, replies[0].port_list, size_info);
  data.payload = (uint8_t *)reply;

  EXPECT_EQ(bcmp_topology_init(), BmOK);

  bm_timer_start_fake.return_val = BmOK;
  bcmp_tx_fake.return_val = BmOK;
  EXPECT_EQ(bcmp_request_neighbor_table(node_id, &addr), BmOK);

  // Variable SENT_REQUEST currently un-reachable, cannot test full function
  packet_process_invoke(BcmpNeighborTableReplyMessage, data);

  // Test failures
  bcmp_tx_fake.return_val = BmEBADMSG;
  EXPECT_NE(bcmp_request_neighbor_table(node_id, &addr), BmOK);
  bm_timer_start_fake.return_val = BmENOMEM;
  EXPECT_NE(bcmp_request_neighbor_table(node_id, &addr), BmOK);
  RESET_FAKE(bcmp_tx);

  topology_builder_cleanup();
  bm_free(reply);
  packet_cleanup();
}

/*!
 @brief Test request message callback
*/
TEST_F(topology_test, request_reply) {
  BcmpNeighborTableRequest request = {
      (uint64_t)RND.rnd_int(UINT32_MAX, UINT16_MAX),
  };
  BcmpProcessData data;
  data.payload = (uint8_t *)&request;

  EXPECT_EQ(bcmp_topology_init(), BmOK);

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

/*!
 @brief Test printing the nodes of a topology
 */
TEST_F(topology_test, print) {
  networkTopology_t topology;
  topology_builder_helper(&topology);

  network_topology_print(&topology);

  topology_builder_cleanup();
}
