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

class Topology : public ::testing::Test {
public:
  rnd_gen RND;
  static void topology_cb(NetworkTopology *topology) {}

private:
protected:
  BcmpNeighborTableReply *replies;
  NeighborTableEntry *entries;
  uint32_t size_info;
  uint32_t size_reply;
  Topology() {}
  ~Topology() override {}
  void SetUp() override {}
  void TearDown() override {}

  /*!
    @brief Build A Random Topology

    @details This builds a random length topology with each node having
             two ports

    @param topology pointer to topology struct
  */
  void topology_builder_helper(NetworkTopology *topology) {
    uint8_t num_nodes = (uint8_t)RND.rnd_int(UINT8_MAX, UINT8_MAX / 2);
    uint8_t num_ports = 2;
    size_info =
        num_ports * sizeof(BcmpPortInfo) + num_ports * sizeof(BcmpNeighborInfo);
    size_reply = sizeof(BcmpNeighborTableReply) + size_info;
    uint64_t next_node_id = 0;
    uint64_t prev_node_id = 0;
    entries =
        (NeighborTableEntry *)bm_malloc(num_nodes * sizeof(NeighborTableEntry));
    replies = (BcmpNeighborTableReply *)bm_malloc(num_nodes * size_reply);
    BcmpPortInfo *info =
        (BcmpPortInfo *)bm_malloc(num_ports * sizeof(BcmpPortInfo));
    BcmpNeighborInfo *neighbors =
        (BcmpNeighborInfo *)bm_malloc(num_ports * sizeof(BcmpNeighborInfo));
    BcmpNeighborTableReply *p;
    BcmpNeighborTableReply *prev = NULL;

    for (int16_t i = num_nodes - 1; i >= 0; i--) {
      p = (BcmpNeighborTableReply *)((uint8_t *)replies + (i * size_reply));
      if (i == num_nodes - 1) {
        p->node_id = (uint64_t)RND.rnd_int(UINT64_MAX, 1);
      } else {
        prev = (BcmpNeighborTableReply *)((uint8_t *)p + size_reply);
        prev_node_id = prev->node_id;
        p->node_id = next_node_id;
      }
      next_node_id = (uint64_t)RND.rnd_int(UINT64_MAX, 1);
      if (i != num_nodes - 1) {
        p->port_len = 2;
        p->neighbor_len = 2;
      } else {
        p->port_len = 2;
        p->neighbor_len = 1;
      }
      for (uint8_t j = 0; j < num_ports; j++) {
        info[j].type = 0;
        info[j].state = true;
        neighbors[j].port = j;
        if (i < num_nodes - 1 && j == 0) {
          neighbors[j].node_id = next_node_id;
          neighbors[j].online = true;
        } else if (i < num_nodes - 1 && j == 1) {
          neighbors[j].node_id = prev_node_id;
          neighbors[j].online = true;
        } else if (i == num_nodes - 1 && j == 0) {
          neighbors[j].node_id = next_node_id;
          neighbors[j].online = true;
        } else {
          neighbors[j].node_id = 0;
          neighbors[j].online = false;
        }
      }
      memcpy(p->port_list, info, sizeof(BcmpPortInfo) * num_ports);
      memcpy(&p->port_list[num_ports], neighbors,
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
      entries[i].neighbor_table_reply = p;
    }
    bm_free(info);
    bm_free(neighbors);
  }

  void topology_builder_cleanup(void) {
    bm_free(entries);
    bm_free(replies);
  }
};

/*!
  @brief Test starting a topology request
*/
TEST_F(Topology, topology_start) {
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
 @brief Test printing the nodes of a topology
 */
TEST_F(Topology, print) {
  NetworkTopology topology;
  topology_builder_helper(&topology);

  network_topology_print(&topology);

  topology_builder_cleanup();
}
