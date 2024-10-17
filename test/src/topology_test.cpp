#include <gtest/gtest.h>
#include <helpers.hpp>

#include "fff.h"

extern "C" {
#include "messages/topology.h"
#include "mock_bcmp.h"
#include "mock_bm_os.h"
#include "mock_packet.h"
}

DEFINE_FFF_GLOBALS;

class topology_test : public ::testing::Test {
public:
  rnd_gen RND;
  static void topology_cb(networkTopology_t *topology) {}

private:
protected:
  topology_test() {}
  ~topology_test() override {}
  void SetUp() override {}
  void TearDown() override {}
};

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

TEST_F(topology_test, request_table) {
  uint32_t addr = (uint32_t)RND.rnd_int(UINT32_MAX, UINT16_MAX);
  uint64_t node_id = (uint64_t)RND.rnd_int(UINT32_MAX, UINT16_MAX);
  BcmpNeighborTableReply reply = {
      (uint64_t)RND.rnd_int(UINT64_MAX, 1),
  };

  EXPECT_EQ(bcmp_topology_init(), BmOK);

  bm_timer_start_fake.return_val = BmOK;
  bcmp_tx_fake.return_val = BmOK;
  EXPECT_EQ(bcmp_request_neighbor_table(node_id, &addr), BmOK);

  //packet_process_invoke(BcmpNeighborTableReplyMessage, NULL);

  // Test failures
  bcmp_tx_fake.return_val = BmEBADMSG;
  EXPECT_NE(bcmp_request_neighbor_table(node_id, &addr), BmOK);
  bm_timer_start_fake.return_val = BmENOMEM;
  EXPECT_NE(bcmp_request_neighbor_table(node_id, &addr), BmOK);
}

TEST_F(topology_test, print) {
  uint8_t num_nodes = (uint8_t)RND.rnd_int(UINT8_MAX, UINT8_MAX / 2);
  uint8_t num_ports = (uint8_t)RND.rnd_int(5, 2);
  uint8_t size_info = num_ports * num_nodes * sizeof(BcmpPortInfo) +
                      num_ports * num_nodes * sizeof(BcmpNeighborInfo);
  uint8_t size_reply = sizeof(BcmpNeighborTableReply) * size_info;
  uint64_t next_node_id = 0;
  NeighborTableEntry *entries =
      (NeighborTableEntry *)bm_malloc(num_nodes * sizeof(NeighborTableEntry));
  BcmpNeighborTableReply *replies =
      (BcmpNeighborTableReply *)bm_malloc(num_nodes * size_reply);
  BcmpPortInfo *info =
      (BcmpPortInfo *)bm_malloc(num_ports * sizeof(BcmpPortInfo));
  BcmpNeighborInfo *neighbors =
      (BcmpNeighborInfo *)bm_malloc(num_ports * sizeof(BcmpNeighborInfo));
  networkTopology_t topology;

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
      topology.back = &entries[i];
      entries[i].nextNode = NULL;
      entries[i].prevNode = NULL;
    } else if (i == 0) {
      entries[i].is_root = true;
      entries[i + 1].prevNode = &entries[i];
      entries[i].nextNode = &entries[i + 1];
      entries[i].prevNode = NULL;
      topology.front = &entries[i];
    } else {
      entries[i].is_root = false;
      entries[i + 1].prevNode = &entries[i];
      entries[i].nextNode = &entries[i + 1];
    }
    entries[i].neighbor_table_reply = &replies[i * size_reply];
  }

  network_topology_print(&topology);

  bm_free(entries);
  bm_free(replies);
  bm_free(info);
  bm_free(neighbors);
}
