#include <gtest/gtest.h>
#include <helpers.hpp>

#include "fff.h"

DEFINE_FFF_GLOBALS

extern "C" {
#include "mock_hash.h"
#include "mock_l2.h"
#include "mock_resource_trie.h"
#include "resource_based_routing.h"
}

#include "util.h"

static ResourceTrieElement WORKING_ELEMENT;

class resource_based_routing_test : public ::testing::Test {
protected:
  rnd_gen RND;
  resource_based_routing_test() {
    WORKING_ELEMENT.resource_id = UINT16_MAX;
    WORKING_ELEMENT.port_mask = RND.rnd_int(UINT16_MAX, 1);
    WORKING_ELEMENT.wildcard_port_mask = RND.rnd_int(UINT16_MAX, 1);
    WORKING_ELEMENT.segment_length = 5;
    WORKING_ELEMENT.local_interest = 1;
    WORKING_ELEMENT.is_wildcard = 0;
    WORKING_ELEMENT.children = NULL;
    WORKING_ELEMENT.sibling = NULL;
    WORKING_ELEMENT.segment = "hello";
  }
  ~resource_based_routing_test() override {}
  void SetUp() override {}
  void TearDown() override {}

  static BmErr fake_match_exact_found(ResourceTrieRoot *root, const char *topic,
                                      BmTopicLength len) {
    (void)topic;
    (void)len;
    root->result.count = 1;
    root->result.matches[0] = &WORKING_ELEMENT;

    return BmOK;
  }

  static BmErr fake_match_exact_not_found(ResourceTrieRoot *root,
                                          const char *topic,
                                          BmTopicLength len) {
    (void)topic;
    (void)len;
    root->result.count = 0;
    root->result.matches[0] = NULL;

    return BmENODATA;
  }
};

TEST_F(resource_based_routing_test, init) {
  static constexpr uint8_t num_ports = 7;
  bm_l2_get_port_count_fake.return_val = num_ports;
  RESET_FAKE(hash_create);
  hash_create_fake.return_val = (Hash *)RND.rnd_int(UINT32_MAX, 1);
  BmErr err = routing_init();
  EXPECT_EQ(err, BmOK);
  uint8_t call_count = hash_create_fake.call_count;
  EXPECT_EQ(call_count, num_ports);
  routing_cleanup();

  RESET_FAKE(hash_create);
  Hash *return_values[3] = {(Hash *)RND.rnd_int(UINT32_MAX, 1),
                            (Hash *)RND.rnd_int(UINT32_MAX, 1), (Hash *)0};
  SET_RETURN_SEQ(hash_create, return_values, array_size(return_values))
  RESET_FAKE(hash_delete);
  err = routing_init();
  EXPECT_EQ(err, BmENOMEM);
  call_count = hash_create_fake.call_count;
  EXPECT_EQ(call_count, array_size(return_values));
  call_count = hash_delete_fake.call_count;
  EXPECT_EQ(call_count, array_size(return_values) - 1);
}

TEST_F(resource_based_routing_test, add_resouce) {
  static constexpr char topic[] = "test/topic/1";
  const BmTopicLength topic_len = strlen(topic);
  ResourceId id;

  // Test adding path
  RESET_FAKE(resource_trie_match_exact);
  RESET_FAKE(resource_trie_add);
  resource_trie_match_exact_fake.return_val = BmOK;
  resource_trie_add_fake.return_val = BmOK;
  BmErr err = add_local_resource(topic, topic_len, &id);
  ASSERT_EQ(err, BmOK);
  uint8_t call_count = resource_trie_add_fake.call_count;
  EXPECT_EQ(call_count, 1);

  // Test existing path
  RESET_FAKE(resource_trie_match_exact);
  RESET_FAKE(resource_trie_add);
  resource_trie_match_exact_fake.custom_fake = fake_match_exact_found;
  err = add_local_resource(topic, topic_len, &id);
  ASSERT_EQ(err, BmOK);
  call_count = resource_trie_add_fake.call_count;
  EXPECT_EQ(call_count, 0);
  EXPECT_EQ(id, WORKING_ELEMENT.resource_id);

  // Test failure paths
  RESET_FAKE(resource_trie_match_exact);
  RESET_FAKE(resource_trie_add);
  resource_trie_match_exact_fake.custom_fake = fake_match_exact_not_found;
  err = add_local_resource(topic, topic_len, &id);
  EXPECT_NE(err, BmOK);
  call_count = resource_trie_add_fake.call_count;
  EXPECT_EQ(call_count, 0);

  RESET_FAKE(resource_trie_match_exact);
  RESET_FAKE(resource_trie_add);
  resource_trie_match_exact_fake.return_val = BmOK;
  resource_trie_add_fake.return_val = BmEBADMSG;
  err = add_local_resource(topic, topic_len, &id);
  EXPECT_NE(err, BmOK);

  err = add_local_resource(NULL, topic_len, &id);
  EXPECT_EQ(err, BmEINVAL);
  err = add_local_resource(topic, 0, &id);
  EXPECT_EQ(err, BmEINVAL);
  err = add_local_resource(topic, topic_len, NULL);
  EXPECT_EQ(err, BmEINVAL);
}
