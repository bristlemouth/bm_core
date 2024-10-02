#include <gtest/gtest.h>
#include <helpers.hpp>

#include "fff.h"

extern "C" {
#include "messages/heartbeat.h"
#include "mock_bcmp.h"
#include "mock_bm_os.h"
#include "mock_info.h"
#include "mock_neighbors.h"
#include "mock_packet.h"
}

#define hb_liveliness_s 10
#define gen_rnd_u64 ((uint64_t)RND.rnd_int(UINT64_MAX, 0))
#define gen_rnd_u32 ((uint32_t)RND.rnd_int(UINT32_MAX, 0))
#define gen_rnd_u16 ((uint16_t)RND.rnd_int(UINT16_MAX, 0))
#define gen_rnd_u8 ((uint8_t)RND.rnd_int(UINT8_MAX, 0))

class heartbeat_test : public ::testing::Test {
public:
  rnd_gen RND;

protected:
  heartbeat_test() {}
  ~heartbeat_test() override {}
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(heartbeat_test, bcmp_send_heartbeat) {
  bcmp_tx_fake.return_val = BmOK;
  ASSERT_EQ(bcmp_send_heartbeat(hb_liveliness_s), BmOK);
  RESET_FAKE(bm_ticks_to_ms);
  RESET_FAKE(bm_get_tick_count);

  bcmp_tx_fake.return_val = BmEBADMSG;
  ASSERT_EQ(bcmp_send_heartbeat(hb_liveliness_s), BmEBADMSG);
  RESET_FAKE(bm_ticks_to_ms);
  RESET_FAKE(bm_get_tick_count);
}

TEST_F(heartbeat_test, bcmp_process_heartbeat) {
  ASSERT_EQ(bcmp_heartbeat_init(), BmOK);
  BcmpNeighbor neighbor = {
      NULL,
      gen_rnd_u64,
      {
          gen_rnd_u8,
          gen_rnd_u8,
          gen_rnd_u8,
          gen_rnd_u8,
          gen_rnd_u8,
          gen_rnd_u8,
          gen_rnd_u8,
          gen_rnd_u8,
          gen_rnd_u8,
          gen_rnd_u8,
          gen_rnd_u8,
          gen_rnd_u8,
      },
      gen_rnd_u8,
      gen_rnd_u32,
      gen_rnd_u64,
      gen_rnd_u32,
      (bool)RND.rnd_int(1, 0),
  };
  BcmpHeartbeat hb = {
      0,
      hb_liveliness_s,
  };
  BcmpProcessData data = {0};
  data.payload = (uint8_t *)&hb;

  // Test neighbor active
  neighbor.last_time_since_boot_us = 0;
  bcmp_update_neighbor_fake.return_val = &neighbor;
  ASSERT_EQ(packet_process_invoke(BcmpHeartbeatMessage, data), BmOK);
  RESET_FAKE(bcmp_update_neighbor);
  RESET_FAKE(bm_get_tick_count);

  // Test neighbor restart/previously not available
  neighbor.last_time_since_boot_us = 1;
  bcmp_update_neighbor_fake.return_val = &neighbor;
  ASSERT_EQ(packet_process_invoke(BcmpHeartbeatMessage, data), BmOK);
  RESET_FAKE(bcmp_update_neighbor);
  RESET_FAKE(bcmp_neighbor_invoke_discovery_cb);
  RESET_FAKE(bcmp_request_info);
  RESET_FAKE(bm_get_tick_count);

  // Test neighbor invalid
  bcmp_update_neighbor_fake.return_val = NULL;
  ASSERT_EQ(packet_process_invoke(BcmpHeartbeatMessage, data), BmEINVAL);
  RESET_FAKE(bcmp_neighbor_invoke_discovery_cb);

  packet_cleanup();
}
