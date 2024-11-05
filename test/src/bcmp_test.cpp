#include <gtest/gtest.h>
#include <helpers.hpp>

#include "fff.h"

DEFINE_FFF_GLOBALS;

extern "C" {
#include "bcmp.h"
#include "mock_bm_adin2111.h"
#include "mock_bm_ip.h"
#include "mock_bm_os.h"
#include "mock_config.h"
#include "mock_dfu.h"
#include "mock_heartbeat.h"
#include "mock_info.h"
#include "mock_neighbors.h"
#include "mock_packet.h"
#include "mock_ping.h"
#include "mock_resource_discovery.h"
#include "mock_time.h"
}

class Bcmp : public ::testing::Test {
public:
  rnd_gen RND;

protected:
  Bcmp() {}
  ~Bcmp() override {}
  void SetUp() override {}
  void TearDown() override {}

  static BmErr bcmp_tx_fake_cb(uint8_t *payload) { return BmOK; }
};

TEST_F(Bcmp, init) {
  // Test success
  bm_task_create_fake.return_val = BmOK;
  NetworkDevice network_device = create_mock_network_device();
  ASSERT_EQ(bcmp_init(network_device), BmOK);
  ASSERT_EQ(bcmp_heartbeat_init_fake.call_count, 1);
  ASSERT_EQ(ping_init_fake.call_count, 1);
  ASSERT_EQ(bm_dfu_init_fake.call_count, 1);
  ASSERT_EQ(bcmp_neighbor_init_fake.call_count, 1);
  ASSERT_EQ(bcmp_device_info_init_fake.call_count, 1);
  ASSERT_EQ(bcmp_resource_discovery_init_fake.call_count, 1);
  ASSERT_EQ(bcmp_config_init_fake.call_count, 1);
  ASSERT_EQ(time_init_fake.call_count, 1);

  RESET_FAKE(bcmp_heartbeat_init);
  RESET_FAKE(ping_init);
  RESET_FAKE(bm_dfu_init);
  RESET_FAKE(bcmp_neighbor_init);
  RESET_FAKE(bcmp_device_info_init);
  RESET_FAKE(bcmp_resource_discovery_init);
  RESET_FAKE(bcmp_config_init);
  RESET_FAKE(time_init);

  // Test failure
  bm_task_create_fake.return_val = BmENOMEM;
  ASSERT_EQ(bcmp_init(network_device), BmENOMEM);
  ASSERT_EQ(bcmp_heartbeat_init_fake.call_count, 1);
  ASSERT_EQ(ping_init_fake.call_count, 1);
  ASSERT_EQ(bm_dfu_init_fake.call_count, 1);
  ASSERT_EQ(bcmp_neighbor_init_fake.call_count, 1);
  ASSERT_EQ(bcmp_device_info_init_fake.call_count, 1);
  ASSERT_EQ(bcmp_resource_discovery_init_fake.call_count, 1);
  ASSERT_EQ(bcmp_config_init_fake.call_count, 1);
  ASSERT_EQ(time_init_fake.call_count, 1);
}

TEST_F(Bcmp, bcmp_tx) {
  const uint32_t dst = RND.rnd_int(UINT32_MAX, 1);
  const uint16_t size =
      RND.rnd_int(max_payload_len - sizeof(BcmpHeader), UINT8_MAX);
  BcmpMessageType type = (BcmpMessageType)RND.rnd_int(UINT8_MAX, 0);
  uint8_t seq_num = RND.rnd_int(UINT8_MAX, 0);
  uint8_t *data = (uint8_t *)bm_malloc(size);

  // Test success
  bm_ip_tx_new_fake.return_val = data;
  serialize_fake.return_val = BmOK;
  bm_ip_tx_perform_fake.return_val = BmOK;
  ASSERT_EQ(bcmp_tx(&dst, type, data, size, seq_num, bcmp_tx_fake_cb), BmOK);
  ASSERT_EQ(bm_ip_tx_cleanup_fake.call_count, 1);
  RESET_FAKE(bm_ip_tx_new);
  RESET_FAKE(serialize);
  RESET_FAKE(bm_ip_tx_perform);
  RESET_FAKE(bm_ip_tx_cleanup);

  // Test failed to transmit
  bm_ip_tx_new_fake.return_val = data;
  serialize_fake.return_val = BmOK;
  bm_ip_tx_perform_fake.return_val = BmEBADMSG;
  ASSERT_EQ(bcmp_tx(&dst, type, data, size, seq_num, bcmp_tx_fake_cb),
            BmEBADMSG);
  ASSERT_EQ(bm_ip_tx_cleanup_fake.call_count, 1);
  RESET_FAKE(bm_ip_tx_new);
  RESET_FAKE(serialize);
  RESET_FAKE(bm_ip_tx_perform);
  RESET_FAKE(bm_ip_tx_cleanup);

  // Test failed to serialize
  bm_ip_tx_new_fake.return_val = data;
  serialize_fake.return_val = BmENODEV;
  ASSERT_EQ(bcmp_tx(&dst, type, data, size, seq_num, bcmp_tx_fake_cb),
            BmENODEV);
  ASSERT_EQ(bm_ip_tx_cleanup_fake.call_count, 1);
  RESET_FAKE(bm_ip_tx_new);
  RESET_FAKE(serialize);
  RESET_FAKE(bm_ip_tx_cleanup);

  // Test no memory available to buffer message
  bm_ip_tx_new_fake.return_val = NULL;
  ASSERT_EQ(bcmp_tx(&dst, type, data, size, seq_num, bcmp_tx_fake_cb),
            BmENOMEM);
  ASSERT_EQ(bm_ip_tx_cleanup_fake.call_count, 0);
  RESET_FAKE(bm_ip_tx_new);

  // Test improper inputs
  ASSERT_EQ(bcmp_tx(NULL, type, data, size, seq_num, bcmp_tx_fake_cb),
            BmEINVAL);
  ASSERT_EQ(bcmp_tx(&dst, type, data, size + max_payload_len, seq_num,
                    bcmp_tx_fake_cb),
            BmEINVAL);

  bm_free(data);
}

TEST_F(Bcmp, bcmp_ll_forward) {
  BcmpHeader header = {};
  const uint16_t size = RND.rnd_int(max_payload_len, UINT8_MAX);
  uint8_t *data = (uint8_t *)bm_malloc(size);
  uint8_t ingress_port = RND.rnd_int(2, 1);

  // Test success
  bm_ip_tx_new_fake.return_val = data;
  bm_ip_tx_perform_fake.return_val = BmOK;
  ASSERT_EQ(bcmp_ll_forward(&header, data, size, ingress_port), BmOK);
  ASSERT_EQ(bm_ip_tx_cleanup_fake.call_count, 1);
  RESET_FAKE(bm_ip_tx_new);
  RESET_FAKE(packet_checksum);
  RESET_FAKE(bm_ip_tx_copy);
  RESET_FAKE(bm_ip_tx_perform);
  RESET_FAKE(bm_ip_tx_cleanup);

  // Test failed to perform
  bm_ip_tx_new_fake.return_val = data;
  bm_ip_tx_perform_fake.return_val = BmEBADMSG;
  ASSERT_EQ(bcmp_ll_forward(&header, data, size, ingress_port), BmEBADMSG);
  ASSERT_EQ(bm_ip_tx_cleanup_fake.call_count, 1);
  RESET_FAKE(bm_ip_tx_new);
  RESET_FAKE(packet_checksum);
  RESET_FAKE(bm_ip_tx_copy);
  RESET_FAKE(bm_ip_tx_perform);
  RESET_FAKE(bm_ip_tx_cleanup);

  // Test no memory available to buffer message
  bm_ip_tx_new_fake.return_val = NULL;
  ASSERT_EQ(bcmp_ll_forward(&header, data, size, ingress_port), BmENOMEM);
  ASSERT_EQ(bm_ip_tx_cleanup_fake.call_count, 0);
  RESET_FAKE(bm_ip_tx_new);

  // Test improper inputs
  ASSERT_EQ(bcmp_ll_forward(NULL, data, size, ingress_port), BmEINVAL);
  ASSERT_EQ(bcmp_ll_forward(&header, NULL, size, ingress_port), BmEINVAL);

  bm_free(data);
}

// Testing a private function, not in the header, but also not static linkage
extern "C" void bcmp_link_change(uint8_t port, bool state);

TEST_F(Bcmp, bcmp_link_change) {
  bcmp_link_change(1, false);

  // Ensure heartbeat is sent on link change
  bcmp_link_change(1, true);
  ASSERT_EQ(bcmp_send_heartbeat_fake.call_count, 1);
  ASSERT_EQ(bm_timer_start_fake.call_count, 1);
  RESET_FAKE(bcmp_send_heartbeat);
  RESET_FAKE(bm_timer_start);
}
