#include <gtest/gtest.h>
#include <helpers.hpp>
#include <string.h>

#include "fff.h"

DEFINE_FFF_GLOBALS;

#define test_topic_0 "example/sub0"
#define test_topic_1 "example/sub1/topic"
#define test_topic_2 "example/**/topic"
#define test_topic_3 "example/*/difficult/str/*/*test"
#define test_topic_3_helper                                                    \
  "example/really/difficult/str/0123456789ABCDEF/here/test"
#define test_topic_3_fail_helper_1 "example/difficult/str/0123456789ABCDEF/test"
#define test_topic_3_fail_helper_2                                             \
  "example/really/difficult/str/0123456789ABCDEF/"
#define test_topic_3_fail_helper_3 "example/really/difficult/str/test"
#define test_topic_4 "example/end/str/*"
#define test_topic_4_helper "example/end/str/0123456789ABCDEF"
#define test_topic_4_fail_helper "example/end/str"
#define test_topic_5 "*/begin/str"
#define test_topic_5_helper "0123456789ABCDEF/begin/str"
#define test_topic_5_fail_helper "begin/str"
#define test_topic_6 "*fouris/*/wildcards/*crazy*"
#define test_topic_6_helper "wowfouris/acrazy/wildcards/amountcrazy/"
#define test_topic_6_fail_helper_1 "wowfouris/acrazy/wildcards/amount"
#define test_topic_6_fail_helper_2 "wowisacrazy/wildcards/amountcrazy/"

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

  void sub_search_helper(const char *str, std::vector<uint32_t> &count) {
    ASSERT_EQ(count.size(), 3);
    uint8_t buf[UINT8_MAX] = {0};
    BmPubSubData *data = NULL;

    RND.rnd_array(buf, array_size(buf));

    data = (BmPubSubData *)bm_malloc(sizeof(BmPubSubData) + strlen(str) +
                                     array_size(buf));
    data->topic_len = strlen(str);
    memcpy((void *)data->topic, str, strlen(str));
    bm_udp_get_payload_fake.return_val = data;
    bm_handle_msg(RND.rnd_int(UINT64_MAX, UINT32_MAX), buf, array_size(buf));
    EXPECT_EQ(CB0_CALLED, count[0]);
    EXPECT_EQ(CB1_CALLED, count[1]);
    EXPECT_EQ(CB2_CALLED, count[2]);
    bm_free(data);
  }
};

TEST_F(PubSub, subscribe) {
  std::vector<uint32_t> count_checks = {1, 1, 1};

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
  sub_search_helper(test_topic_0, count_checks);
  count_checks = {2, 2, 2};
  sub_search_helper(test_topic_1, count_checks);

  // Unsubscribe from all topics
  ASSERT_EQ(bm_unsub(test_topic_0, sub_callback_2), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_0, sub_callback_1), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_0, sub_callback_0), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_1, sub_callback_1), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_1, sub_callback_0), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_1, sub_callback_2), BmOK);

  // Test wildcard subbing, ex: test_topic_2 will match test_topic_1
  bcmp_resource_discovery_add_resource_fake.return_val = BmOK;
  ASSERT_EQ(bm_sub(test_topic_2, sub_callback_0), BmOK);
  bcmp_resource_discovery_add_resource_fake.return_val = BmEAGAIN;
  ASSERT_EQ(bm_sub(test_topic_2, sub_callback_1), BmOK);
  ASSERT_EQ(bm_sub(test_topic_2, sub_callback_2), BmOK);

  count_checks = {3, 3, 3};
  sub_search_helper(test_topic_1, count_checks);
  ASSERT_EQ(bm_unsub(test_topic_2, sub_callback_2), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_2, sub_callback_1), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_2, sub_callback_0), BmOK);

  bcmp_resource_discovery_add_resource_fake.return_val = BmOK;
  ASSERT_EQ(bm_sub(test_topic_3, sub_callback_0), BmOK);
  bcmp_resource_discovery_add_resource_fake.return_val = BmEAGAIN;
  ASSERT_EQ(bm_sub(test_topic_3, sub_callback_1), BmOK);
  ASSERT_EQ(bm_sub(test_topic_3, sub_callback_2), BmOK);

  count_checks = {4, 4, 4};
  sub_search_helper(test_topic_3_helper, count_checks);
  // Ensure failures on strings that do not exactly match pattern
  sub_search_helper(test_topic_3_fail_helper_1, count_checks);
  sub_search_helper(test_topic_3_fail_helper_2, count_checks);
  sub_search_helper(test_topic_3_fail_helper_3, count_checks);

  ASSERT_EQ(bm_unsub(test_topic_3, sub_callback_1), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_3, sub_callback_0), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_3, sub_callback_2), BmOK);

  bcmp_resource_discovery_add_resource_fake.return_val = BmOK;
  ASSERT_EQ(bm_sub(test_topic_4, sub_callback_0), BmOK);
  bcmp_resource_discovery_add_resource_fake.return_val = BmEAGAIN;
  ASSERT_EQ(bm_sub(test_topic_4, sub_callback_1), BmOK);
  ASSERT_EQ(bm_sub(test_topic_4, sub_callback_2), BmOK);

  count_checks = {5, 5, 5};
  sub_search_helper(test_topic_4_helper, count_checks);
  // Ensure failures on strings that does not exactly match pattern
  sub_search_helper(test_topic_4_fail_helper, count_checks);
  ASSERT_EQ(bm_unsub(test_topic_4, sub_callback_1), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_4, sub_callback_0), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_4, sub_callback_2), BmOK);

  bcmp_resource_discovery_add_resource_fake.return_val = BmOK;
  ASSERT_EQ(bm_sub(test_topic_5, sub_callback_0), BmOK);
  bcmp_resource_discovery_add_resource_fake.return_val = BmEAGAIN;
  ASSERT_EQ(bm_sub(test_topic_5, sub_callback_1), BmOK);
  ASSERT_EQ(bm_sub(test_topic_5, sub_callback_2), BmOK);

  count_checks = {6, 6, 6};
  sub_search_helper(test_topic_5_helper, count_checks);
  // Ensure failures on string that does not exactly match pattern
  sub_search_helper(test_topic_5_fail_helper, count_checks);

  ASSERT_EQ(bm_unsub(test_topic_5, sub_callback_1), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_5, sub_callback_0), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_5, sub_callback_2), BmOK);

  bcmp_resource_discovery_add_resource_fake.return_val = BmOK;
  ASSERT_EQ(bm_sub(test_topic_6, sub_callback_0), BmOK);
  bcmp_resource_discovery_add_resource_fake.return_val = BmEAGAIN;
  ASSERT_EQ(bm_sub(test_topic_6, sub_callback_1), BmOK);
  ASSERT_EQ(bm_sub(test_topic_6, sub_callback_2), BmOK);

  count_checks = {7, 7, 7};
  sub_search_helper(test_topic_6_helper, count_checks);
  // Ensure failures on strings that do not exactly match pattern
  sub_search_helper(test_topic_6_fail_helper_1, count_checks);
  sub_search_helper(test_topic_6_fail_helper_2, count_checks);

  ASSERT_EQ(bm_unsub(test_topic_6, sub_callback_1), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_6, sub_callback_0), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_6, sub_callback_2), BmOK);

  // Test to make sure wildcard does not cross over strings
  bcmp_resource_discovery_add_resource_fake.return_val = BmOK;
  ASSERT_EQ(bm_sub(test_topic_2, sub_callback_0), BmOK);
  ASSERT_EQ(bm_sub(test_topic_3, sub_callback_1), BmOK);
  ASSERT_EQ(bm_sub(test_topic_6, sub_callback_2), BmOK);
  count_checks = {7, 8, 7};
  sub_search_helper(test_topic_3_helper, count_checks);
  sub_search_helper(test_topic_4_helper, count_checks);
  sub_search_helper(test_topic_5_helper, count_checks);
  count_checks = {8, 8, 7};
  sub_search_helper(test_topic_1, count_checks);
  sub_search_helper(test_topic_0, count_checks);
  count_checks = {8, 8, 8};
  sub_search_helper(test_topic_6_helper, count_checks);
  ASSERT_EQ(bm_unsub(test_topic_2, sub_callback_0), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_3, sub_callback_1), BmOK);
  ASSERT_EQ(bm_unsub(test_topic_6, sub_callback_2), BmOK);

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

TEST_F(PubSub, wildcard_match) {
  ASSERT_TRUE(bm_wildcard_match("aaaa", 4, "a*a", 3));
}
