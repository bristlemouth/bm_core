#include <gtest/gtest.h>
#include <helpers.hpp>

#include "fff.h"
#include "util.h"

DEFINE_FFF_GLOBALS;

extern "C" {
#include "messages/resource_discovery.h"
#include "mock_bcmp.h"
#include "mock_bm_os.h"
#include "mock_device.h"
#include "mock_packet.h"
}

static uint32_t CALL_COUNT = 0;

class ResourceDiscovery : public ::testing::Test {
public:
  rnd_gen RND;

private:
protected:
  ResourceDiscovery() {}
  ~ResourceDiscovery() override {}
  void SetUp() override { CALL_COUNT = 0; }
  void TearDown() override {}
  static void resource_discovery_cb(void *arg) {
    (void)arg;
    CALL_COUNT++;
  }
};

TEST_F(ResourceDiscovery, resource_process_request) {
  BcmpResourceTableRequest request = {(uint64_t)RND.rnd_int(UINT64_MAX, 1)};
  BcmpProcessData data = {0};
  data.payload = (uint8_t *)&request;
  const char *resources_pub[] = {
      "bcmp/resource/pub1", "bcmp/resource/pub2", "bcmp/resource/pub3",
      "bcmp/resource/pub4", "bcmp/resource/pub5", "bcmp/resource/pub6",
      "bcmp/resource/pub7", "bcmp/resource/pub8",
  };
  const char *resources_sub[] = {
      "bcmp/resource/sub1", "bcmp/resource/sub2", "bcmp/resource/sub3",
      "bcmp/resource/sub4", "bcmp/resource/sub5", "bcmp/resource/sub6",
      "bcmp/resource/sub7", "bcmp/resource/sub8",
  };

  // Initialize callbacks
  bm_mutex_create_fake.return_val = (uint64_t *)RND.rnd_int(UINT64_MAX, 1);
  bm_semaphore_take_fake.return_val = BmOK;
  bm_semaphore_give_fake.return_val = BmOK;
  ASSERT_EQ(bcmp_resource_discovery_init(), BmOK);

  // Add resources to discovery
  for (size_t i = 0; i < array_size(resources_pub); i++) {
    ASSERT_EQ(bcmp_resource_discovery_add_resource(
                  resources_pub[i], strlen(resources_pub[i]), PUB,
                  default_resource_add_timeout_ms),
              BmOK);
  }
  for (size_t i = 0; i < array_size(resources_sub); i++) {
    ASSERT_EQ(bcmp_resource_discovery_add_resource(
                  resources_sub[i], strlen(resources_sub[i]), SUB,
                  default_resource_add_timeout_ms),
              BmOK);
  }

  // Test successful process request
  bcmp_tx_fake.return_val = BmOK;
  node_id_fake.return_val = request.target_node_id;
  ASSERT_EQ(packet_process_invoke(BcmpResourceTableRequestMessage, data), BmOK);
  ASSERT_EQ(bcmp_tx_fake.call_count, 1);
  RESET_FAKE(bcmp_tx);

  // Test wrong node for request
  node_id_fake.return_val = 0;
  ASSERT_NE(packet_process_invoke(BcmpResourceTableRequestMessage, data), BmOK);
  ASSERT_EQ(bcmp_tx_fake.call_count, 0);

  // Test failure to transmit
  bcmp_tx_fake.return_val = BmEBADMSG;
  node_id_fake.return_val = request.target_node_id;
  ASSERT_NE(packet_process_invoke(BcmpResourceTableRequestMessage, data), BmOK);
  ASSERT_EQ(bcmp_tx_fake.call_count, 1);
  RESET_FAKE(bcmp_tx);

  // Initialize to cleanup list
  ASSERT_EQ(bcmp_resource_discovery_init(), BmOK);
  packet_cleanup();
}

TEST_F(ResourceDiscovery, resource_process_reply) {
  const char *resources_pub[] = {
      "bcmp/resource/pub1", "bcmp/resource/pub2", "bcmp/resource/pub3",
      "bcmp/resource/pub4", "bcmp/resource/pub5", "bcmp/resource/pub6",
      "bcmp/resource/pub7", "bcmp/resource/pub8",
  };
  const char *resources_sub[] = {
      "bcmp/resource/sub1", "bcmp/resource/sub2", "bcmp/resource/sub3",
      "bcmp/resource/sub4", "bcmp/resource/sub5", "bcmp/resource/sub6",
      "bcmp/resource/sub7", "bcmp/resource/sub8",
  };
  uint64_t target_node_id = 0;
  BcmpResourceTableReply *reply = NULL;
  BcmpProcessData data = {0};
  uint32_t msg_len = 0;
  uint32_t offset = 0;
  // Add resources to increase message length
  for (size_t i = 0; i < array_size(resources_pub); i++) {
    msg_len += sizeof(BcmpResource) + strlen(resources_pub[i]);
  }
  for (size_t i = 0; i < array_size(resources_sub); i++) {
    msg_len += sizeof(BcmpResource) + strlen(resources_sub[i]);
  }
  reply = (BcmpResourceTableReply *)bm_malloc(sizeof(BcmpResourceTableReply) +
                                              msg_len);
  // Node_id is set to zero when calculating ip_to_nodeid
  reply->node_id = 0;

  // Setup data necessary to reply to
  reply->num_pubs = array_size(resources_pub);
  reply->num_subs = array_size(resources_sub);
  for (size_t i = 0; i < array_size(resources_pub); i++) {
    uint16_t len = strlen(resources_pub[i]);
    memcpy(&reply->resource_list[offset], &len, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy(&reply->resource_list[offset], resources_pub[i],
           strlen(resources_pub[i]));
    offset += strlen(resources_pub[i]);
  }
  for (size_t i = 0; i < array_size(resources_sub); i++) {
    uint16_t len = strlen(resources_sub[i]);
    memcpy(&reply->resource_list[offset], &len, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy(&reply->resource_list[offset], resources_sub[i],
           strlen(resources_sub[i]));
    offset += strlen(resources_sub[i]);
  }
  data.payload = (uint8_t *)reply;

  // Initialize callbacks
  bm_mutex_create_fake.return_val = (uint64_t *)RND.rnd_int(UINT64_MAX, 1);
  bm_semaphore_take_fake.return_val = BmOK;
  bm_semaphore_give_fake.return_val = BmOK;
  ASSERT_EQ(bcmp_resource_discovery_init(), BmOK);

  // Test successful process reply
  bcmp_tx_fake.return_val = BmOK;
  ASSERT_EQ(bcmp_resource_discovery_send_request(target_node_id, NULL), BmOK);
  ASSERT_EQ(packet_process_invoke(BcmpResourceTableReplyMessage, data), BmOK);
  ASSERT_EQ(bcmp_tx_fake.call_count, 1);
  RESET_FAKE(bcmp_tx);

  // Test unsuccessful send request
  bcmp_tx_fake.return_val = BmEBADMSG;
  ASSERT_NE(bcmp_resource_discovery_send_request(target_node_id, NULL), BmOK);
  ASSERT_EQ(bcmp_tx_fake.call_count, 1);
  RESET_FAKE(bcmp_tx);

  // Test wrong node ID (reset and run at the end to clear ll item)
  reply->node_id = 0xff;
  ASSERT_EQ(bcmp_resource_discovery_send_request(target_node_id, NULL), BmOK);
  ASSERT_EQ(bcmp_tx_fake.call_count, 1);
  ASSERT_EQ(packet_process_invoke(BcmpResourceTableReplyMessage, data), BmOK);
  reply->node_id = 0;
  ASSERT_EQ(packet_process_invoke(BcmpResourceTableReplyMessage, data), BmOK);
  RESET_FAKE(bcmp_tx);

  // Test 0 pubs and 0 subs
  reply->num_pubs = array_size(resources_pub);
  reply->num_subs = array_size(resources_sub);
  bcmp_tx_fake.return_val = BmOK;
  ASSERT_EQ(bcmp_resource_discovery_send_request(target_node_id, NULL), BmOK);
  ASSERT_EQ(bcmp_tx_fake.call_count, 1);
  ASSERT_EQ(packet_process_invoke(BcmpResourceTableReplyMessage, data), BmOK);
  RESET_FAKE(bcmp_tx);

  // Test callback
  bcmp_tx_fake.return_val = BmOK;
  ASSERT_EQ(bcmp_resource_discovery_send_request(target_node_id,
                                                 resource_discovery_cb),
            BmOK);
  ASSERT_EQ(bcmp_tx_fake.call_count, 1);
  ASSERT_EQ(packet_process_invoke(BcmpResourceTableReplyMessage, data), BmOK);
  RESET_FAKE(bcmp_tx);
  ASSERT_EQ(CALL_COUNT, 1);

  // Initialize to cleanup list
  ASSERT_EQ(bcmp_resource_discovery_init(), BmOK);
  packet_cleanup();
  free(reply);
}

TEST_F(ResourceDiscovery, resource_adding_getting) {
  bool found = false;
  uint16_t num_resources = 0;
  const char *resources_pub[] = {
      "bcmp/resource/pub1", "bcmp/resource/pub2", "bcmp/resource/pub3",
      "bcmp/resource/pub4", "bcmp/resource/pub5", "bcmp/resource/pub6",
      "bcmp/resource/pub7", "bcmp/resource/pub8",
  };
  const char *resources_sub[] = {
      "bcmp/resource/sub1", "bcmp/resource/sub2", "bcmp/resource/sub3",
      "bcmp/resource/sub4", "bcmp/resource/sub5", "bcmp/resource/sub6",
      "bcmp/resource/sub7", "bcmp/resource/sub8", "bcmp/resource/sub9",
  };
  const char *resources_not = "not/in/resources";

  // Print/discover resources that do not exist yet (branch coverage)
  bcmp_resource_discovery_print_resources();
  BcmpResourceTableReply *reply = bcmp_resource_discovery_get_local_resources();
  ASSERT_NE(reply, nullptr);
  bm_free(reply);

  // Add resources to discovery
  for (size_t i = 0; i < array_size(resources_pub); i++) {
    ASSERT_EQ(bcmp_resource_discovery_add_resource(
                  resources_pub[i], strlen(resources_pub[i]), PUB,
                  default_resource_add_timeout_ms),
              BmOK);
  }
  for (size_t i = 0; i < array_size(resources_sub); i++) {
    ASSERT_EQ(bcmp_resource_discovery_add_resource(
                  resources_sub[i], strlen(resources_sub[i]), SUB,
                  default_resource_add_timeout_ms),
              BmOK);
  }
  ASSERT_EQ(bcmp_resource_discovery_add_resource(
                resources_sub[0], strlen(resources_sub[0]), SUB,
                default_resource_add_timeout_ms),
            BmEAGAIN);

  // Test finding resources
  ASSERT_EQ(bcmp_resource_discovery_find_resource(
                resources_pub[0], strlen(resources_pub[0]), &found, PUB,
                default_resource_add_timeout_ms),
            BmOK);
  ASSERT_EQ(found, true);
  ASSERT_EQ(bcmp_resource_discovery_find_resource(
                resources_not, strlen(resources_not), &found, PUB,
                default_resource_add_timeout_ms),
            BmOK);
  ASSERT_EQ(found, false);

  // Get number of resources
  ASSERT_EQ(bcmp_resource_discovery_get_num_resources(
                &num_resources, PUB, default_resource_add_timeout_ms),
            BmOK);
  ASSERT_EQ(num_resources, array_size(resources_pub));
  ASSERT_EQ(bcmp_resource_discovery_get_num_resources(
                &num_resources, SUB, default_resource_add_timeout_ms),
            BmOK);
  ASSERT_EQ(num_resources, array_size(resources_sub));

  bcmp_resource_discovery_print_resources();

  reply = bcmp_resource_discovery_get_local_resources();
  ASSERT_NE(reply, nullptr);
  bm_free(reply);
}
