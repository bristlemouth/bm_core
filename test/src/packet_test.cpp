#include <gtest/gtest.h>
#include <helpers.hpp>
#include <stddef.h>
#include <string.h>

#include "fff.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "mock_bm_os.h"
#include "packet.h"
#ifdef __cplusplus
}
#endif

#define MAX_PAYLOAD_SIZE UINT16_MAX
#define MIN_PAYLOAD_SIZE 8192
#define GEN_RND_U64 ((uint64_t)RND.rnd_int(UINT64_MAX, 0))
#define GEN_RND_U32 ((uint32_t)RND.rnd_int(UINT32_MAX, 0))
#define GEN_RND_U16 ((uint16_t)RND.rnd_int(UINT16_MAX, 0))
#define GEN_RND_U8 ((uint8_t)RND.rnd_int(UINT8_MAX, 0))

DECLARE_FAKE_VALUE_FUNC(BmErr, bcmp_process_heartbeat, BcmpParserData);
DEFINE_FAKE_VALUE_FUNC(BmErr, bcmp_process_heartbeat, BcmpParserData);

static rnd_gen RND;

class packet_test : public ::testing::Test {
public:
  typedef struct {
    uint8_t *payload;
    uint32_t src_addr[4];
    uint32_t dst_addr[4];
  } PacketTestData;
  uint8_t *test_payload;
  size_t test_payload_size;

protected:
  packet_test() {}
  ~packet_test() override {}
  static void *get_data(void *payload) {
    PacketTestData *ret = (PacketTestData *)payload;
    return (void *)ret->payload;
  }
  static void *src_ip(void *payload) {
    PacketTestData *ret = (PacketTestData *)payload;
    return (void *)ret->src_addr;
  }
  static void *dst_ip(void *payload) {
    PacketTestData *ret = (PacketTestData *)payload;
    return (void *)ret->dst_addr;
  }
  static uint16_t calc_checksum(void *payload, uint32_t size) {
    uint32_t sum = 0;
    uint8_t *data = (uint8_t *)get_data(payload);

    for (size_t i = 0; i < size; i += 2) {
      uint16_t word = data[i] | (data[i + 1] << 8);
      sum += word;
      if (sum > 0xFFFF) {
        sum = (sum & 0xFFFF) + (sum >> 16);
      }
    }

    // If the length is odd, add the remaining byte as a 16-bit word with zero padding
    if (size % 2 != 0) {
      uint16_t word = data[size - 1];
      sum += word;
      if (sum > 0xFFFF) {
        sum = (sum & 0xFFFF) + (sum >> 16);
      }
    }

    // Return the one's complement of the sum
    return (uint16_t)~sum;
  }
  void SetUp() override {
    test_payload_size = RND.rnd_int(MAX_PAYLOAD_SIZE, MIN_PAYLOAD_SIZE);
    test_payload = (uint8_t *)calloc(test_payload_size, sizeof(uint8_t));

    RESET_FAKE(bm_semaphore_create);
    RESET_FAKE(bm_timer_create);
    RESET_FAKE(bm_timer_start);
    bm_semaphore_create_fake.return_val =
        (BmSemaphore)RND.rnd_int(UINT32_MAX, UINT16_MAX);
    bm_timer_create_fake.return_val =
        (BmTimer)RND.rnd_int(UINT32_MAX, UINT16_MAX);
    bm_timer_start_fake.return_val = BmOK;
    ASSERT_EQ(packet_init(src_ip, dst_ip, get_data, calc_checksum), BmOK);

    static BcmpPacketCfg heartbeat_packet = {
        sizeof(BcmpHeartbeat),
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
  BcmpHeader payload_stuffer(uint8_t *data, void *payload, uint32_t size,
                             BcmpMessageType type, uint32_t seq_req) {
    BcmpHeader header = {
        GEN_RND_U16, GEN_RND_U16, GEN_RND_U8, GEN_RND_U8,
        GEN_RND_U32, GEN_RND_U8,  GEN_RND_U8, GEN_RND_U8,
    };
    header.type = type;
    header.seq_num = seq_req;
    memcpy(data, (void *)&header, sizeof(BcmpHeader));
    memcpy((void *)(data + sizeof(BcmpHeader)), payload, sizeof(BcmpHeartbeat));
    return header;
  }
};

TEST_F(packet_test, serialize) {
  PacketTestData data;
  data.payload = test_payload;
  RND.rnd_array((uint8_t *)data.src_addr, sizeof(data.src_addr));
  RND.rnd_array((uint8_t *)data.dst_addr, sizeof(data.dst_addr));

  // Test a normal message
  BcmpHeartbeat hb = {
      GEN_RND_U64,
      GEN_RND_U32,
  };

  ASSERT_EQ(
      serialize((void *)&data, (void *)&hb, BcmpHeartbeatMessage, 0, NULL),
      BmOK);
  ASSERT_EQ(((BcmpHeader *)data.payload)->type, BcmpHeartbeatMessage);
  ASSERT_EQ(memcmp((void *)(data.payload + sizeof(BcmpHeader)), (void *)&hb,
                   sizeof(BcmpHeartbeat)),
            BmOK);

  // Test sequence reply
  memset(data.payload, 0, test_payload_size);
}

TEST_F(packet_test, parse) {
  PacketTestData data;
  BcmpHeader header;
  data.payload = test_payload;
  RND.rnd_array((uint8_t *)data.src_addr, sizeof(data.src_addr));
  RND.rnd_array((uint8_t *)data.dst_addr, sizeof(data.dst_addr));

  // Test a normal message
  RESET_FAKE(bcmp_process_heartbeat);
  BcmpHeartbeat hb = {
      GEN_RND_U64,
      GEN_RND_U32,
  };

  header = payload_stuffer(data.payload, (void *)&hb, sizeof(hb),
                           BcmpHeartbeatMessage, 0);
  bcmp_process_heartbeat_fake.return_val = BmOK;
  ASSERT_EQ(parse((void *)&data), BmOK);
  ASSERT_EQ(bcmp_process_heartbeat_fake.call_count, 1);
  ASSERT_EQ(memcmp((void *)(bcmp_process_heartbeat_fake.arg0_val.header),
                   (void *)&header, sizeof(BcmpHeader)),
            BmOK);
}

DECLARE_FAKE_VALUE_FUNC(BmErr, bcmp_sequence_request, uint8_t *);
DEFINE_FAKE_VALUE_FUNC(BmErr, bcmp_sequence_request, uint8_t *);

TEST_F(packet_test, sequence_request) {
  BcmpPacketCfg request_neighbor_info_packet = {
      sizeof(BcmpNeighborInfo),
      false,
      true,
      NULL,
  };
  BcmpPacketCfg reply_neighbor_info_packet = {
      sizeof(BcmpNeighborInfo),
      true,
      false,
      NULL,
  };
  BcmpNeighborInfo request_neighbor_info = {
      GEN_RND_U64,
      GEN_RND_U8,
      GEN_RND_U8,
  };
  BcmpNeighborInfo reply_neighbor_info = {
      GEN_RND_U64,
      GEN_RND_U8,
      GEN_RND_U8,
  };
  PacketTestData data;
  BcmpHeader header;
  size_t iterations = RND.rnd_int(UINT16_MAX, UINT8_MAX);

  data.payload = test_payload;
  RND.rnd_array((uint8_t *)data.src_addr, sizeof(data.src_addr));
  RND.rnd_array((uint8_t *)data.dst_addr, sizeof(data.dst_addr));
  RESET_FAKE(bcmp_sequence_request);

  ASSERT_EQ(packet_add(&request_neighbor_info_packet,
                       BcmpNeighborProtoRequestMessage),
            BmOK);
  ASSERT_EQ(
      packet_add(&reply_neighbor_info_packet, BcmpNeighborProtoReplyMessage),
      BmOK);

  bm_semaphore_take_fake.return_val = BmOK;
  bm_semaphore_give_fake.return_val = BmOK;
  for (size_t i = 0; i < iterations; i++) {
    ASSERT_EQ(serialize((void *)&data, (void *)&request_neighbor_info,
                        BcmpNeighborProtoRequestMessage, 0,
                        bcmp_sequence_request),
              0);
  }

  for (size_t i = 0; i < iterations; i++) {
    header = payload_stuffer(data.payload, (void *)&reply_neighbor_info,
                             sizeof(reply_neighbor_info),
                             BcmpNeighborProtoReplyMessage, i);
    bcmp_sequence_request_fake.return_val = BmOK;
    ASSERT_EQ(parse((void *)&data), BmOK);
    ASSERT_EQ(bcmp_sequence_request_fake.call_count, i + 1);
  }
}
