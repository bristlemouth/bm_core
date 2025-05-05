#include <gtest/gtest.h>
#include <helpers.hpp>
#include <string.h>

#include "fff.h"

DEFINE_FFF_GLOBALS;

#define test_topic_0 "example/sub0"
#define test_topic_1 "example/sub1/topic"

extern "C" {
#include "mock_bm_ip.h"
#include "mock_bm_os.h"
#include "mock_middleware.h"
#include "mock_resource_discovery.h"
#include "pubsub.h"
}

static uint8_t CB0_CALLED;
static uint8_t CB1_CALLED;
static uint8_t CB2_CALLED;

class PubSub : public ::testing::Test {
protected:
  rnd_gen RND;
  PubSub() {}
  ~PubSub() override {}
  void SetUp() override {
    CB0_CALLED = 0;
    CB1_CALLED = 0;
    CB2_CALLED = 0;
  }
  void TearDown() override {}
  static void sub_callback_0(uint64_t node_id, const char *topic,
                             uint16_t topic_len, const uint8_t *data,
                             uint16_t data_len, uint8_t type, uint8_t version) {
    (void)node_id;
    (void)topic;
    (void)topic_len;
    (void)data;
    (void)data_len;
    (void)type;
    (void)version;
    CB0_CALLED++;
  }
  static void sub_callback_1(uint64_t node_id, const char *topic,
                             uint16_t topic_len, const uint8_t *data,
                             uint16_t data_len, uint8_t type, uint8_t version) {
    (void)node_id;
    (void)topic;
    (void)topic_len;
    (void)data;
    (void)data_len;
    (void)type;
    (void)version;
    CB1_CALLED++;
  }

  static void sub_callback_2(uint64_t node_id, const char *topic,
                             uint16_t topic_len, const uint8_t *data,
                             uint16_t data_len, uint8_t type, uint8_t version) {
    (void)node_id;
    (void)topic;
    (void)topic_len;
    (void)data;
    (void)data_len;
    (void)type;
    (void)version;
    CB2_CALLED++;
  }
};

TEST_F(PubSub, subscribe) {
  uint8_t buf[UINT8_MAX] = {0};
  BmPubSubData *data = NULL;

  RND.rnd_array(buf, array_size(buf));

  // Add topics and associate multiple callbacks with topics
  bcmp_resource_discovery_add_resource_fake.return_val = BmOK;
  ASSERT_EQ(bm_sub(test_topic_0, sub_callback_0), BmOK);
  bcmp_resource_discovery_add_resource_fake.return_val = BmEAGAIN;
  ASSERT_EQ(bm_sub(test_topic_0, sub_callback_1), BmOK);
  ASSERT_EQ(bm_sub(test_topic_0, sub_callback_2), BmOK);
  bcmp_resource_discovery_add_resource_fake.return_val = BmOK;
  ASSERT_EQ(bm_sub(test_topic_1, sub_callback_0), BmOK);
  bcmp_resource_discovery_add_resource_fake.return_val = BmEAGAIN;
  ASSERT_EQ(bm_sub(test_topic_1, sub_callback_1), BmOK);
  ASSERT_EQ(bm_sub(test_topic_1, sub_callback_2), BmOK);

  // Test making sure that callbacks are called properly
  data = (BmPubSubData *)bm_malloc(sizeof(BmPubSubData) + strlen(test_topic_0) +
                                   array_size(buf));
  data->topic_len = strlen(test_topic_0);
  memcpy((void *)data->topic, test_topic_0, strlen(test_topic_0));
  bm_udp_get_payload_fake.return_val = data;
  bm_handle_msg(RND.rnd_int(UINT64_MAX, UINT32_MAX), buf, array_size(buf));
  ASSERT_EQ(CB0_CALLED, 1);
  ASSERT_EQ(CB1_CALLED, 1);
  ASSERT_EQ(CB2_CALLED, 1);
  bm_free(data);

  data = (BmPubSubData *)bm_malloc(sizeof(BmPubSubData) + strlen(test_topic_1) +
                                   array_size(buf));
  data->topic_len = strlen(test_topic_1);
  memcpy((void *)data->topic, test_topic_1, strlen(test_topic_1));
  bm_udp_get_payload_fake.return_val = data;
  bm_handle_msg(RND.rnd_int(UINT64_MAX, UINT32_MAX), buf, array_size(buf));
  ASSERT_EQ(CB0_CALLED, 2);
  ASSERT_EQ(CB1_CALLED, 2);
  ASSERT_EQ(CB2_CALLED, 2);
  bm_free(data);

  // Unsubscribe from all topics
  ASSERT_EQ(bm_unsub(test_topic_0, sub_callback_2), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_0, sub_callback_1), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_0, sub_callback_0), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_1, sub_callback_1), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_1, sub_callback_0), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_1, sub_callback_2), BmOK);

  // Test improperly adding topics
  ASSERT_NE(bm_sub(test_topic_0, NULL), BmOK);
  ASSERT_NE(bm_sub(NULL, sub_callback_0), BmOK);
  ASSERT_NE(bm_sub_wl(test_topic_0, 0, sub_callback_0), BmOK);
  ASSERT_NE(bm_sub_wl(test_topic_0, BM_TOPIC_MAX_LEN, sub_callback_0), BmOK);
  bcmp_resource_discovery_add_resource_fake.return_val = BmENOMEM;
  ASSERT_NE(bm_sub(test_topic_0, sub_callback_0), BmOK);

  // Test adding a callback that already exists
  bcmp_resource_discovery_add_resource_fake.return_val = BmOK;
  ASSERT_EQ(bm_sub(test_topic_1, sub_callback_0), BmOK);
  ASSERT_EQ(bm_sub(test_topic_1, sub_callback_0), BmOK);

  RESET_FAKE(bcmp_resource_discovery_add_resource);
}

TEST_F(PubSub, unsubscribe) {
  // Test proper unsub
  bcmp_resource_discovery_add_resource_fake.return_val = BmOK;
  ASSERT_EQ(bm_sub(test_topic_0, sub_callback_0), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_0, sub_callback_0), BmOK);

  // Test improper unsub
  ASSERT_EQ(bm_sub(test_topic_0, sub_callback_0), BmOK);
  ASSERT_NE(bm_unsub(NULL, sub_callback_0), BmOK);
  ASSERT_NE(bm_unsub(test_topic_0, NULL), BmOK);
  ASSERT_NE(bm_unsub_wl(test_topic_0, BM_TOPIC_MAX_LEN, sub_callback_0), BmOK);

  // Test no matching callback
  ASSERT_EQ(bm_unsub(test_topic_0, sub_callback_1), BmENOENT);

  ASSERT_EQ(bm_unsub(test_topic_0, sub_callback_0), BmOK);
  RESET_FAKE(bcmp_resource_discovery_add_resource);
}

TEST_F(PubSub, publish) {
  uint8_t buf[UINT8_MAX] = {0};
  uint8_t type = RND.rnd_int(UINT8_MAX, 0);
  uint8_t version = RND.rnd_int(UINT8_MAX, 0);
  const char *topic = test_topic_1;
  void *message =
      (void *)bm_malloc(sizeof(BmPubSubData) + array_size(buf) + strlen(topic));

  RND.rnd_array(buf, array_size(buf));

  // Test proper use case
  bcmp_resource_discovery_add_resource_fake.return_val = BmOK;
  bm_udp_new_fake.return_val = message;
  bm_udp_get_payload_fake.return_val = message;
  bm_middleware_net_tx_fake.return_val = BmOK;
  ASSERT_EQ(bm_pub(topic, buf, array_size(buf), type, version), BmOK);

  // Test if self subscribed to published topic
  ASSERT_EQ(bm_sub(topic, sub_callback_0), BmOK);
  ASSERT_EQ(bm_pub(topic, buf, array_size(buf), type, version), BmOK);
  ASSERT_EQ(bm_unsub(topic, sub_callback_0), BmOK);

  // Test publishing without data (ensure it doesn't break with strange use cases)
  ASSERT_EQ(bm_pub(topic, NULL, 0, type, version), BmOK);
  ASSERT_EQ(bm_pub(topic, NULL, array_size(buf), type, version), BmOK);
  ASSERT_EQ(bm_pub(topic, buf, 0, type, version), BmOK);

  // Test improper use cases
  ASSERT_NE(bm_pub(NULL, buf, array_size(buf), type, version), BmOK);
  ASSERT_NE(bm_pub_wl(topic, 0, buf, array_size(buf), type, version), BmOK);

  ASSERT_NE(
      bm_pub_wl(topic, BM_TOPIC_MAX_LEN, buf, array_size(buf), type, version),
      BmOK);

  free(message);
  RESET_FAKE(bcmp_resource_discovery_add_resource);
}

TEST_F(PubSub, utility) {
  bm_print_subs();

  bcmp_resource_discovery_add_resource_fake.return_val = BmOK;
  ASSERT_EQ(bm_sub(test_topic_0, sub_callback_0), BmOK);
  ASSERT_EQ(bm_sub(test_topic_0, sub_callback_1), BmOK);
  ASSERT_EQ(bm_sub(test_topic_0, sub_callback_2), BmOK);
  ASSERT_EQ(bm_sub(test_topic_1, sub_callback_0), BmOK);
  ASSERT_EQ(bm_sub(test_topic_1, sub_callback_1), BmOK);
  ASSERT_EQ(bm_sub(test_topic_1, sub_callback_2), BmOK);

  bm_get_subs();

  ASSERT_EQ(bm_unsub(test_topic_0, sub_callback_2), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_0, sub_callback_1), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_0, sub_callback_0), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_1, sub_callback_1), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_1, sub_callback_0), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_1, sub_callback_2), BmOK);
  RESET_FAKE(bcmp_resource_discovery_add_resource);
}
