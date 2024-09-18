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
    bm_semaphore_create_fake.return_val = (BmSemaphore)RND.rnd_int(UINT32_MAX, UINT16_MAX);
    bm_timer_create_fake.return_val = (BmTimer)RND.rnd_int(UINT32_MAX, UINT16_MAX);
    bm_timer_start_fake.return_val = BmOK;
    ASSERT_EQ(packet_init(src_ip, dst_ip, get_data, calc_checksum), BmOK);

    static BcmpPacketCfg heartbeat_packet = {
        sizeof(BcmpHeartbeat),
        false,
        NULL,
        bcmp_process_heartbeat,
    };
    ASSERT_EQ(packet_add(&heartbeat_packet, BcmpHeartbeatMessage), BmOK);
  }
  void TearDown() override {
    packet_remove(BcmpHeartbeatMessage);
    free(test_payload);
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

  ASSERT_EQ(serialize((void *)&data, (void *)&hb, BcmpHeartbeatMessage, 0), 0);
  ASSERT_EQ(((BcmpHeader *)data.payload)->type, BcmpHeartbeatMessage);
  ASSERT_EQ(
      memcmp((void *)(data.payload + sizeof(BcmpHeader)), (void *)&hb, sizeof(BcmpHeartbeat)),
      0);

  // Test sequence reply
  memset(data.payload, 0, test_payload_size);
}

TEST_F(packet_test, parse) {
  PacketTestData data;
  data.payload = test_payload;
  RND.rnd_array((uint8_t *)data.src_addr, sizeof(data.src_addr));
  RND.rnd_array((uint8_t *)data.dst_addr, sizeof(data.dst_addr));
  BcmpHeader header = {
      GEN_RND_U16, GEN_RND_U16, GEN_RND_U8, GEN_RND_U8,
      GEN_RND_U32, GEN_RND_U8,  GEN_RND_U8, GEN_RND_U8,
  };

  // Test a normal message
  RESET_FAKE(bcmp_process_heartbeat);
  BcmpHeartbeat hb = {
      GEN_RND_U64,
      GEN_RND_U32,
  };

  header.type = BcmpHeartbeatMessage;
  memcpy((void *)data.payload, (void *)&header, sizeof(BcmpHeader));
  memcpy((void *)(data.payload + sizeof(BcmpHeader)), (void *)&hb, sizeof(BcmpHeartbeat));
  bcmp_process_heartbeat_fake.return_val = BmOK;
  ASSERT_EQ(parse((void *)&data), BmOK);
  ASSERT_EQ(bcmp_process_heartbeat_fake.call_count, 1);
  ASSERT_EQ(memcmp((void *)(bcmp_process_heartbeat_fake.arg0_val.header), (void *)&header,
                   sizeof(BcmpHeader)),
            0);
}
