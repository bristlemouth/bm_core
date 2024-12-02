#include <gtest/gtest.h>
#include <helpers.hpp>

#include "fff.h"

DEFINE_FFF_GLOBALS;

extern "C" {
#include "messages/ping.h"
#include "mock_bcmp.h"
#include "mock_bm_os.h"
#include "mock_device.h"
#include "mock_packet.h"
}

class Ping : public ::testing::Test {
public:
  rnd_gen RND;

private:
protected:
  Ping() {}
  ~Ping() override {}
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(Ping, request_ping) {
  BcmpEchoRequest request = {
      (uint64_t)RND.rnd_int(UINT64_MAX, UINT8_MAX),
      (uint16_t)RND.rnd_int(UINT16_MAX, UINT8_MAX),
      (uint16_t)RND.rnd_int(UINT16_MAX, UINT8_MAX),
      0,
  };
  BcmpProcessData data = {0};
  data.payload = (uint8_t *)&request;

  // Initialize processing callbacks
  ASSERT_EQ(ping_init(), BmOK);

  // Test successful request process
  bcmp_tx_fake.return_val = BmOK;
  node_id_fake.return_val = request.target_node_id;
  ASSERT_EQ(packet_process_invoke(BcmpEchoRequestMessage, data), BmOK);
  RESET_FAKE(bcmp_tx);

  // Test request process with failure to transmit
  bcmp_tx_fake.return_val = BmEBADMSG;
  ASSERT_EQ(packet_process_invoke(BcmpEchoRequestMessage, data), BmEBADMSG);
  ASSERT_EQ(bcmp_tx_fake.call_count, 1);
  RESET_FAKE(bcmp_tx);
  RESET_FAKE(node_id);

  // Test request process with wrong node id
  node_id_fake.return_val = 0;
  ASSERT_EQ(packet_process_invoke(BcmpEchoRequestMessage, data), BmENOTINTREC);
  ASSERT_EQ(bcmp_tx_fake.call_count, 0);
  RESET_FAKE(node_id);

  // Test request process with target id of 0
  bcmp_tx_fake.return_val = BmOK;
  node_id_fake.return_val = request.target_node_id;
  ASSERT_EQ(packet_process_invoke(BcmpEchoRequestMessage, data), BmOK);
  ASSERT_EQ(bcmp_tx_fake.call_count, 1);
  RESET_FAKE(node_id);
  RESET_FAKE(bcmp_tx);

  packet_cleanup();
}

TEST_F(Ping, reply_ping) {
  const char *str = "Ping Test String";
  BcmpEchoReply *reply =
      (BcmpEchoReply *)bm_malloc(sizeof(BcmpEchoRequest) + strlen(str));
  BcmpEchoRequest request = {
      (uint64_t)RND.rnd_int(UINT64_MAX, UINT8_MAX),
      (uint16_t)RND.rnd_int(UINT16_MAX, UINT8_MAX),
      (uint16_t)RND.rnd_int(UINT16_MAX, UINT8_MAX),
      0,
  };
  BcmpProcessData data = {0};
  memcpy(&reply->payload[0], str, strlen(str));
  reply->id = request.id;
  reply->node_id = request.target_node_id;
  reply->seq_num = request.seq_num;
  reply->payload_len = strlen(str);
  data.payload = (uint8_t *)reply;

  // Initialize processing callbacks
  ASSERT_EQ(ping_init(), BmOK);

  // Test proper use case
  node_id_fake.return_val = request.id;
  ASSERT_EQ(bcmp_send_ping_request(request.target_node_id, NULL,
                                   (const uint8_t *)str, strlen(str)),
            BmOK);
  ASSERT_EQ(bcmp_tx_fake.call_count, 1);
  RESET_FAKE(bcmp_tx);
  bm_ticks_to_ms_fake.return_val = 100;
  ASSERT_EQ(packet_process_invoke(BcmpEchoReplyMessage, data), BmOK);
  RESET_FAKE(bm_ticks_to_ms)

  // Run again to test replacment of expected payload
  ASSERT_EQ(bcmp_send_ping_request(request.target_node_id, NULL,
                                   (const uint8_t *)str, strlen(str)),
            BmOK);
  ASSERT_EQ(bcmp_tx_fake.call_count, 1);
  RESET_FAKE(bcmp_tx);
  bm_ticks_to_ms_fake.return_val = 100;
  ASSERT_EQ(packet_process_invoke(BcmpEchoReplyMessage, data), BmOK);
  RESET_FAKE(bm_ticks_to_ms)

  free(reply);
}
