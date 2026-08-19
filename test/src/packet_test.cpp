#include <gtest/gtest.h>
#include <helpers.hpp>
#include <stddef.h>
#include <string.h>

#include "fff.h"

DEFINE_FFF_GLOBALS;

extern "C" {
#include "mock_bm_os.h"
#include "packet.h"
}

#define max_payload_size UINT16_MAX
#define min_payload_size 8192
#define gen_rnd_u64 ((uint64_t)RND.rnd_int(UINT64_MAX, 0))
#define gen_rnd_u32 ((uint32_t)RND.rnd_int(UINT32_MAX, 0))
#define gen_rnd_u16 ((uint16_t)RND.rnd_int(UINT16_MAX, 0))
#define gen_rnd_u8 ((uint8_t)RND.rnd_int(UINT8_MAX, 0))

DECLARE_FAKE_VALUE_FUNC(BmErr, bcmp_process_heartbeat, BcmpProcessData);
DEFINE_FAKE_VALUE_FUNC(BmErr, bcmp_process_heartbeat, BcmpProcessData);
DECLARE_FAKE_VALUE_FUNC(BmErr, send_packet, void *);
DEFINE_FAKE_VALUE_FUNC(BmErr, send_packet, void *);

static rnd_gen RND;
static bool fail_checksum;
static uint32_t ref;
static BmTimerCb timer_cb;

class Packet : public ::testing::Test {
public:
  typedef struct {
    uint8_t *payload;
    uint8_t src_addr[16];
    uint8_t dst_addr[16];
  } PacketTestData;
  uint8_t *test_payload;
  size_t test_payload_size;

protected:
  Packet() {}
  ~Packet() override {}
  static void *get_data(void *payload) {
    PacketTestData *ret = (PacketTestData *)payload;
    return (void *)ret->payload;
  }
  static BmIpAddr *src_ip(void *payload) {
    PacketTestData *ret = (PacketTestData *)payload;
    return (BmIpAddr *)&ret->src_addr;
  }
  static BmIpAddr *dst_ip(void *payload) {
    PacketTestData *ret = (PacketTestData *)payload;
    return (BmIpAddr *)&ret->dst_addr;
  }
  static void increment_ref(void *payload) {
    (void)payload;
    ref++;
  }

  static void decrement_ref(void *payload) {
    (void)payload;
    ref--;
  }

  static uint16_t calc_checksum(void *payload, uint32_t size) {
    if (fail_checksum) {
      return 100;
    } else {
      return 0;
    }
  }

  static BmTimer bm_timer_create_stub(const char *name, uint32_t period_ms,
                                      bool auto_reload, void *time_id,
                                      BmTimerCb cb) {
    (void)name;
    (void)period_ms;
    (void)auto_reload;
    (void)time_id;

    timer_cb = cb;

    return (BmTimer)bm_timer_create_fake.return_val;
  }

  void SetUp() override {
    ref = 0;
    fail_checksum = false;
    test_payload_size = RND.rnd_int(max_payload_size, min_payload_size);
    test_payload = (uint8_t *)calloc(test_payload_size, sizeof(uint8_t));

    RESET_FAKE(bm_mutex_create);
    RESET_FAKE(bm_timer_create);
    RESET_FAKE(bm_timer_start);
    RESET_FAKE(bcmp_process_heartbeat);
    bm_mutex_create_fake.return_val =
        (BmSemaphore)RND.rnd_int(UINT32_MAX, UINT16_MAX);
    bm_timer_create_fake.return_val =
        (BmTimer)RND.rnd_int(UINT32_MAX, UINT16_MAX);
    bm_timer_create_fake.custom_fake = bm_timer_create_stub;
    bm_timer_start_fake.return_val = BmOK;
    ASSERT_EQ(packet_init((BcmpPacketCb){
                  .src_ip = src_ip,
                  .dst_ip = dst_ip,
                  .data = get_data,
                  .checksum = calc_checksum,
                  .increment = increment_ref,
                  .decrement = decrement_ref,
                  .send = send_packet,
              }),
              BmOK);

    static BcmpPacketCfg heartbeat_packet = {
        false,
        false,
        bcmp_process_heartbeat,
    };
    ASSERT_EQ(packet_add(&heartbeat_packet, BcmpHeartbeatMessage), BmOK);
  }
  void TearDown() override {
    packet_remove(BcmpHeartbeatMessage);
    free(test_payload);
  }

  /*!
   @brief Stuffs A Payload With Header Information And Data

   @param payload payload to be stuffed
   @param data data to stuff in the payload
   @param size size of the data to stuff into the payload
   @param type type of message that is to be stuffed
   @param seq_req sequence request of a message (if applicable)

   @return The header of the stuffed payload
   */
  BcmpHeader payload_stuffer(uint8_t *payload, void *data, uint32_t size,
                             BcmpMessageType type, uint32_t seq_req) {
    BcmpHeader header = {
        gen_rnd_u16, 0,          gen_rnd_u8, gen_rnd_u8,
        gen_rnd_u32, gen_rnd_u8, gen_rnd_u8, gen_rnd_u8,
    };
    header.type = type;
    header.seq_num = seq_req;
    memcpy(payload, (void *)&header, sizeof(BcmpHeader));
    memcpy((void *)(payload + sizeof(BcmpHeader)), data, sizeof(BcmpHeartbeat));
    return header;
  }
};

/*!
 @brief Test Serialize Functionality Of Packet Module

 @details This is responsible for testing the serialization of messages from
          BCMP. This test specifically generates a random heartbeat message,
          serializes the message, ensures the payload matches this random
          message and the header populates the correct type of message
 */
TEST_F(Packet, serialize) {
  PacketTestData data;
  data.payload = test_payload;
  RND.rnd_array((uint8_t *)data.src_addr, sizeof(data.src_addr));
  RND.rnd_array((uint8_t *)data.dst_addr, sizeof(data.dst_addr));

  // Test a normal message
  BcmpHeartbeat hb = {
      gen_rnd_u64,
      gen_rnd_u32,
  };

  ASSERT_EQ(serialize((void *)&data, (void *)&hb, sizeof(hb),
                      BcmpHeartbeatMessage, 0, packet_null_cb()),
            BmOK);
  ASSERT_EQ(((BcmpHeader *)data.payload)->type, BcmpHeartbeatMessage);
  ASSERT_EQ(memcmp((void *)(data.payload + sizeof(BcmpHeader)), (void *)&hb,
                   sizeof(BcmpHeartbeat)),
            BmOK);

  // Test sequence reply
  memset(data.payload, 0, test_payload_size);
}

/*!
 @brief Test Parsing Functionality Of Packet Module

 @details This is responsible for creating a fake heartbeat message
          with header, parsing the message, and ensuring the callback has
          actually been called, as well as ensuring the data to the
          callback message is what was generated
 */
TEST_F(Packet, process) {
  PacketTestData data;
  BcmpHeader header;
  data.payload = test_payload;
  RND.rnd_array((uint8_t *)data.src_addr, sizeof(data.src_addr));
  RND.rnd_array((uint8_t *)data.dst_addr, sizeof(data.dst_addr));

  // Test a normal message
  BcmpHeartbeat hb = {
      gen_rnd_u64,
      gen_rnd_u32,
  };

  header = payload_stuffer(data.payload, (void *)&hb, sizeof(hb),
                           BcmpHeartbeatMessage, 0);
  bcmp_process_heartbeat_fake.return_val = BmOK;
  ASSERT_EQ(process_received_message((void *)&data, sizeof(hb)), BmOK);
  ASSERT_EQ(bcmp_process_heartbeat_fake.call_count, 1);
  ASSERT_EQ(memcmp((void *)(bcmp_process_heartbeat_fake.arg0_val.header),
                   (void *)&header, sizeof(BcmpHeader)),
            BmOK);
}

DECLARE_FAKE_VALUE_FUNC(BmErr, bcmp_sequence_request_full, BcmpProcessData);
DEFINE_FAKE_VALUE_FUNC(BmErr, bcmp_sequence_request_full, BcmpProcessData);
DECLARE_FAKE_VALUE_FUNC(BmErr, bcmp_sequence_request, uint8_t *);
DEFINE_FAKE_VALUE_FUNC(BmErr, bcmp_sequence_request, uint8_t *);
DECLARE_FAKE_VALUE_FUNC(BmErr, bcmp_neighbor_info, BcmpProcessData);
DEFINE_FAKE_VALUE_FUNC(BmErr, bcmp_neighbor_info, BcmpProcessData);

/*!
 @brief Test Sequence Requests

 @details Tests a sequence request of a message with and without a
          callback function to ensure that items are added to the
          linked list properly and removed from the linked list.
          This ensures the callback function is getting called the
          expected number of times for both cases mentioned above.
 */
TEST_F(Packet, sequence_request) {
  BcmpPacketCfg request_neighbor_info_packet = {
      false,
      true,
      bcmp_neighbor_info,
  };
  BcmpPacketCfg reply_neighbor_info_packet = {
      true,
      false,
      bcmp_neighbor_info,
  };
  BcmpNeighborInfo request_neighbor_info = {
      gen_rnd_u64,
      gen_rnd_u8,
      gen_rnd_u8,
  };
  BcmpNeighborInfo reply_neighbor_info = {
      gen_rnd_u64,
      gen_rnd_u8,
      gen_rnd_u8,
  };
  PacketTestData data;
  BcmpHeader header;
  size_t iterations = RND.rnd_int(UINT16_MAX, UINT8_MAX);

  bm_ticks_to_ms_fake.return_val = 0;
  data.payload = test_payload;
  RND.rnd_array((uint8_t *)data.src_addr, sizeof(data.src_addr));
  RND.rnd_array((uint8_t *)data.dst_addr, sizeof(data.dst_addr));

  // Test with callback
  RESET_FAKE(bcmp_sequence_request);
  RESET_FAKE(bcmp_neighbor_info);
  ASSERT_EQ(packet_add(&request_neighbor_info_packet,
                       BcmpNeighborProtoRequestMessage),
            BmOK);
  ASSERT_EQ(
      packet_add(&reply_neighbor_info_packet, BcmpNeighborProtoReplyMessage),
      BmOK);

  bm_semaphore_take_fake.return_val = BmOK;
  bm_semaphore_give_fake.return_val = BmOK;
  for (size_t i = 0; i < iterations; i++) {
    uint32_t ref_before = ref;
    ASSERT_EQ(serialize((void *)&data, (void *)&request_neighbor_info,
                        sizeof(request_neighbor_info),
                        BcmpNeighborProtoRequestMessage, 0,
                        packet_payload_cb(bcmp_sequence_request)),
              0);
    ASSERT_EQ(((BcmpHeader *)data.payload)->seq_num, i);
    // Make sure reference gets updated
    EXPECT_LT(ref_before, ref);
  }

  for (size_t i = 0; i < iterations; i++) {
    uint32_t ref_before = ref;
    header = payload_stuffer(data.payload, (void *)&reply_neighbor_info,
                             sizeof(reply_neighbor_info),
                             BcmpNeighborProtoReplyMessage, i);
    bcmp_sequence_request_fake.return_val = BmOK;
    ASSERT_EQ(
        process_received_message((void *)&data, sizeof(reply_neighbor_info)),
        BmOK);
    ASSERT_EQ(bcmp_sequence_request_fake.call_count, i + 1);
    ASSERT_EQ(bcmp_neighbor_info_fake.call_count, 0);
    // Make sure reference gets updated
    EXPECT_GT(ref_before, ref);
  }

  // Test without callback
  RESET_FAKE(bcmp_sequence_request);
  RESET_FAKE(bcmp_neighbor_info);

  bm_semaphore_take_fake.return_val = BmOK;
  bm_semaphore_give_fake.return_val = BmOK;
  for (size_t i = 0; i < iterations; i++) {
    ASSERT_EQ(serialize((void *)&data, (void *)&request_neighbor_info,
                        sizeof(request_neighbor_info),
                        BcmpNeighborProtoRequestMessage, 0, packet_null_cb()),
              0);
    ASSERT_EQ(((BcmpHeader *)data.payload)->seq_num, i + iterations);
  }

  for (size_t i = 0; i < iterations; i++) {
    header = payload_stuffer(data.payload, (void *)&reply_neighbor_info,
                             sizeof(reply_neighbor_info),
                             BcmpNeighborProtoReplyMessage, i + iterations);
    ASSERT_EQ(
        process_received_message((void *)&data, sizeof(reply_neighbor_info)),
        BmOK);
    ASSERT_EQ(bcmp_sequence_request_fake.call_count, 0);
    ASSERT_EQ(bcmp_neighbor_info_fake.call_count, i + 1);
  }

  iterations *= 2;
  // Test timeout and retry logic for sequenced request
  RESET_FAKE(bcmp_neighbor_info);
  ASSERT_EQ(serialize((void *)&data, (void *)&request_neighbor_info,
                      sizeof(request_neighbor_info),
                      BcmpNeighborProtoRequestMessage, 0, packet_null_cb()),
            BmOK);
  for (size_t i = 0; i < packet_retry_count; i++) {
    RESET_FAKE(send_packet);
    // Continue updating tick offset to force a timeout
    bm_ticks_to_ms_fake.return_val += UINT16_MAX;
    timer_cb(NULL);
    EXPECT_EQ(send_packet_fake.call_count, 1);
  }
  // Should not send again
  uint32_t ref_before = ref;
  bm_ticks_to_ms_fake.return_val += UINT16_MAX;
  RESET_FAKE(send_packet);
  timer_cb(NULL);
  EXPECT_EQ(send_packet_fake.call_count, 0);
  // Make sure reference gets decremented
  EXPECT_GT(ref_before, ref);
  // Make sure process callback is invoked after timeout
  ASSERT_EQ(bcmp_neighbor_info_fake.call_count, 1);
  BcmpProcessData expected_timeout_data = {0};
  int cmp = memcmp(&bcmp_neighbor_info_fake.arg0_val, &expected_timeout_data,
                   sizeof(BcmpProcessData));
  EXPECT_EQ(cmp, 0);

  // Test full callback logic
  RESET_FAKE(bcmp_sequence_request_full);
  ASSERT_EQ(serialize((void *)&data, (void *)&request_neighbor_info,
                      sizeof(request_neighbor_info),
                      BcmpNeighborProtoRequestMessage, 0,
                      packet_full_cb(bcmp_sequence_request_full)),
            BmOK);
  // Make sure sequence number is updated to match request
  iterations++;
  header = payload_stuffer(data.payload, (void *)&reply_neighbor_info,
                           sizeof(reply_neighbor_info),
                           BcmpNeighborProtoReplyMessage, iterations);
  bcmp_sequence_request_full_fake.return_val = BmOK;
  ASSERT_EQ(
      process_received_message((void *)&data, sizeof(reply_neighbor_info)),
      BmOK);
  EXPECT_EQ(bcmp_sequence_request_full_fake.call_count, 1);

  packet_remove(BcmpNeighborProtoRequestMessage);
  packet_remove(BcmpNeighborProtoReplyMessage);
}

TEST_F(Packet, sequence_reply) {
  BcmpPacketCfg reply_neighbor_info_packet = {
      true,
      false,
      bcmp_neighbor_info,
  };
  BcmpNeighborInfo reply_neighbor_info = {
      gen_rnd_u64,
      gen_rnd_u8,
      gen_rnd_u8,
  };
  PacketTestData data;
  data.payload = test_payload;
  RND.rnd_array((uint8_t *)data.src_addr, sizeof(data.src_addr));
  RND.rnd_array((uint8_t *)data.dst_addr, sizeof(data.dst_addr));

  ASSERT_EQ(
      packet_add(&reply_neighbor_info_packet, BcmpNeighborProtoReplyMessage),
      BmOK);

  ASSERT_EQ(serialize((void *)&data, (void *)&reply_neighbor_info,
                      sizeof(reply_neighbor_info),
                      BcmpNeighborProtoReplyMessage, 0, packet_null_cb()),
            0);
  ASSERT_EQ(((BcmpHeader *)data.payload)->seq_num, 0);
}

TEST_F(Packet, null_payload) {
  PacketTestData data = {};
  RND.rnd_array((uint8_t *)data.src_addr, sizeof(data.src_addr));
  RND.rnd_array((uint8_t *)data.dst_addr, sizeof(data.dst_addr));
  // Test a completely null payload
  ASSERT_EQ(process_received_message((void *)&data, sizeof(BcmpHeartbeat)),
            BmEINVAL);
}

TEST_F(Packet, fail_process_checksum) {
  PacketTestData data;
  BcmpHeader header;
  fail_checksum = true;
  data.payload = test_payload;
  RND.rnd_array((uint8_t *)data.src_addr, sizeof(data.src_addr));
  RND.rnd_array((uint8_t *)data.dst_addr, sizeof(data.dst_addr));

  // Test a normal message
  BcmpHeartbeat hb = {
      gen_rnd_u64,
      gen_rnd_u32,
  };

  header = payload_stuffer(data.payload, (void *)&hb, sizeof(hb),
                           BcmpHeartbeatMessage, 0);
  bcmp_process_heartbeat_fake.return_val = BmOK;
  ASSERT_EQ(process_received_message((void *)&data, sizeof(hb)), BmEBADMSG);
  ASSERT_EQ(bcmp_process_heartbeat_fake.call_count, 0);
}
