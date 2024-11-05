#include <gtest/gtest.h>
#include <helpers.hpp>

#include "fff.h"

DEFINE_FFF_GLOBALS;

extern "C" {
#include "util.h"
#include "messages/time.h"
#include "mock_bcmp.h"
#include "mock_bm_os.h"
#include "mock_device.h"
#include "mock_packet.h"
#include "mock_bm_rtc.h"
}

class Time : public ::testing::Test {
public:
  rnd_gen RND;

  private:
  protected:
    Time() {}
    ~Time() override {}
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(Time, request_time)
{
  bm_rtc_get_micro_seconds_fake.return_val = 0;

  BcmpSystemTimeHeader time_header = {
      (uint64_t)RND.rnd_int(UINT64_MAX, UINT8_MAX),
      (uint64_t)RND.rnd_int(UINT64_MAX, UINT8_MAX),
  };
  BcmpSystemTimeSet req_msg = {
      time_header,
  };
  BcmpHeader header = {
      .type = BcmpSystemTimeRequestMessage,
  };
  BcmpProcessData data = {0};
  data.payload = (uint8_t *)&req_msg;
  data.header = &header;

  // Intialize processing callbacks
  ASSERT_EQ(time_init(), BmOK);

  // Test successful request process
  bcmp_tx_fake.return_val = BmOK;
  node_id_fake.return_val = time_header.target_node_id;
  bm_rtc_get_fake.return_val = BmOK;
  ASSERT_EQ(packet_process_invoke(BcmpSystemTimeRequestMessage, data), BmOK);
  ASSERT_EQ(bcmp_tx_fake.call_count, 1);
  ASSERT_EQ(bm_rtc_get_fake.call_count, 1);
  RESET_FAKE(bcmp_tx);
  RESET_FAKE(node_id);
  RESET_FAKE(bm_rtc_get);

  // Test request process with failure to transmit
  bcmp_tx_fake.return_val = BmEBADMSG;
  bm_rtc_get_fake.return_val = BmOK;
  node_id_fake.return_val = time_header.target_node_id;
  ASSERT_EQ(packet_process_invoke(BcmpSystemTimeRequestMessage, data), BmOK);
  ASSERT_EQ(bcmp_tx_fake.call_count, 1);
  ASSERT_EQ(bm_rtc_get_fake.call_count, 1);
  RESET_FAKE(bcmp_tx);
  RESET_FAKE(node_id);
  RESET_FAKE(bm_rtc_get);

  // Test request process with wrong node id - should forward
  node_id_fake.return_val = 0;
  ASSERT_EQ(packet_process_invoke(BcmpSystemTimeRequestMessage, data), BmOK);
  ASSERT_EQ(bcmp_tx_fake.call_count, 0);
  ASSERT_EQ(bcmp_ll_forward_fake.call_count, 1);
  RESET_FAKE(node_id);

  // Test request process with target id of 0
  bcmp_tx_fake.return_val = BmOK;
  node_id_fake.return_val = time_header.target_node_id;
  req_msg.header.target_node_id = 0;
  ASSERT_EQ(packet_process_invoke(BcmpSystemTimeRequestMessage, data), BmEINVAL);
  ASSERT_EQ(bcmp_tx_fake.call_count, 0);

  // Test failure to get time
  bcmp_tx_fake.return_val = BmOK;
  node_id_fake.return_val = time_header.target_node_id;
  req_msg.header.target_node_id = time_header.target_node_id;
  bm_rtc_get_fake.return_val = BmEINVAL;
  ASSERT_EQ(packet_process_invoke(BcmpSystemTimeRequestMessage, data), BmOK);
  ASSERT_EQ(bcmp_tx_fake.call_count, 0);
  ASSERT_EQ(bm_rtc_get_fake.call_count, 1);
  RESET_FAKE(node_id);
  RESET_FAKE(bm_rtc_get);
  RESET_FAKE(bcmp_tx);

  packet_cleanup();
}

TEST_F(Time, time_response) {
  BcmpSystemTimeHeader time_header = {
      (uint64_t)RND.rnd_int(UINT64_MAX, UINT8_MAX),
      (uint64_t)RND.rnd_int(UINT64_MAX, UINT8_MAX),
  };
  BcmpSystemTimeSet resp_msg = {
      time_header,
      0,
  };
  BcmpHeader header = {
      .type = BcmpSystemTimeResponseMessage,
  };
  BcmpProcessData data = {0};
  data.payload = (uint8_t *)&resp_msg;
  data.header = &header;

  // Used to check if the target node id is correct in both tests here
  node_id_fake.return_val = time_header.target_node_id;

  // Intialize processing callbacks
  ASSERT_EQ(time_init(), BmOK);

  // Test a successful response
  ASSERT_EQ(packet_process_invoke(BcmpSystemTimeResponseMessage, data), BmOK);

  // Test a response with the wrong target node id
  resp_msg.header.target_node_id = 0;
  ASSERT_EQ(packet_process_invoke(BcmpSystemTimeResponseMessage, data), BmEINVAL);
  RESET_FAKE(node_id);

  packet_cleanup();
}

TEST_F(Time, set_time) {
  BcmpSystemTimeHeader header = {
      (uint64_t)RND.rnd_int(UINT64_MAX, UINT8_MAX),
      (uint64_t)RND.rnd_int(UINT64_MAX, UINT8_MAX),
  };
  BcmpSystemTimeSet set_msg = {
      header,
      0,
  };
  BcmpHeader bcmp_header = {
      .type = BcmpSystemTimeSetMessage,
  };
  BcmpProcessData data = {0};
  data.payload = (uint8_t *)&set_msg;
  data.header = &bcmp_header;

  // Use this node id for all the of the tests
  node_id_fake.return_val = header.target_node_id;

  // Intialize processing callbacks
  ASSERT_EQ(time_init(), BmOK);

  // Test successful set process
  bcmp_tx_fake.return_val = BmOK;
  bm_rtc_set_fake.return_val = BmOK;
  ASSERT_EQ(packet_process_invoke(BcmpSystemTimeSetMessage, data), BmOK);
  ASSERT_EQ(bcmp_tx_fake.call_count, 1);
  ASSERT_EQ(bm_rtc_set_fake.call_count, 1);
  RESET_FAKE(bm_rtc_set);
  RESET_FAKE(bcmp_tx);

  // Test set with failure to set time
  bm_rtc_set_fake.return_val = BmEINVAL;
  ASSERT_EQ(packet_process_invoke(BcmpSystemTimeSetMessage, data), BmOK);
  ASSERT_EQ(bcmp_tx_fake.call_count, 0);
  ASSERT_EQ(bm_rtc_set_fake.call_count, 1);
  RESET_FAKE(bm_rtc_set);

  // Test set with a node id of 0
  bcmp_tx_fake.return_val = BmOK;
  bm_rtc_set_fake.return_val = BmOK;
  set_msg.header.target_node_id = 0;
  ASSERT_EQ(packet_process_invoke(BcmpSystemTimeSetMessage, data), BmOK);
  ASSERT_EQ(bcmp_tx_fake.call_count, 1);
  ASSERT_EQ(bm_rtc_set_fake.call_count, 1);
  RESET_FAKE(bm_rtc_set);
  RESET_FAKE(bcmp_tx);
}
