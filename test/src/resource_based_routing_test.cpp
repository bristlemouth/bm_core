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
  resource_based_routing_test() {}
  ~resource_based_routing_test() override {}
  void SetUp() override {
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

  static BmErr fake_add_success(ResourceTrieRoot *root, const char *topic,
                                BmTopicLength len, ResourceId id,
                                uint16_t port_mask, bool local_interest) {
    (void)topic;
    (void)len;
    root->result.count = 1;
    root->result.matches[0] = &WORKING_ELEMENT;

    return BmOK;
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

TEST_F(resource_based_routing_test, add_local_resouce) {
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

  // Test invalid input
  err = add_local_resource(NULL, topic_len, &id);
  EXPECT_EQ(err, BmEINVAL);
  err = add_local_resource(topic, 0, &id);
  EXPECT_EQ(err, BmEINVAL);
  err = add_local_resource(topic, topic_len, NULL);
  EXPECT_EQ(err, BmEINVAL);
}

TEST_F(resource_based_routing_test, add_neighbor_resouce) {
  static constexpr char topic[] = "test/topic/1";
  const BmTopicLength topic_len = strlen(topic);
  const ResourceId neighbor_id = RND.rnd_int(0xFFFFF, 1);
  ResourceId id = neighbor_id;
  const uint8_t port_num = RND.rnd_int(16, 1);

  bm_l2_get_port_count_fake.return_val = 16;

  // Test adding path
  RESET_FAKE(resource_trie_match_exact);
  RESET_FAKE(resource_trie_add);
  RESET_FAKE(hash_insert);
  resource_trie_match_exact_fake.return_val = BmOK;
  resource_trie_add_fake.custom_fake = fake_add_success;
  hash_insert_fake.return_val = BmOK;
  BmErr err = add_neighbor_resource(topic, topic_len, &id, port_num);
  ASSERT_EQ(err, BmOK);
  uint8_t call_count = resource_trie_add_fake.call_count;
  EXPECT_EQ(call_count, 1);
  call_count = hash_insert_fake.call_count;
  EXPECT_EQ(call_count, 1);
  uint16_t port_mask = 1 << (port_num - 1);
  EXPECT_TRUE(WORKING_ELEMENT.port_mask & port_mask);
  WORKING_ELEMENT.port_mask &= ~port_mask;

  // Test trie existing path
  RESET_FAKE(resource_trie_match_exact);
  RESET_FAKE(resource_trie_add);
  RESET_FAKE(hash_insert);
  resource_trie_match_exact_fake.custom_fake = fake_match_exact_found;
  hash_insert_fake.return_val = BmOK;
  err = add_neighbor_resource(topic, topic_len, &id, port_num);
  ASSERT_EQ(err, BmOK);
  call_count = resource_trie_add_fake.call_count;
  EXPECT_EQ(call_count, 0);
  call_count = hash_insert_fake.call_count;
  EXPECT_EQ(call_count, 1);
  EXPECT_TRUE(WORKING_ELEMENT.port_mask & port_mask);
  WORKING_ELEMENT.port_mask &= ~port_mask;

  // Test hash existing path
  RESET_FAKE(hash_insert);
  RESET_FAKE(hash_remove);
  hash_remove_fake.return_val = BmOK;
  BmErr insert_return_values[2] = {BmEALREADY, BmOK};
  SET_RETURN_SEQ(hash_insert, insert_return_values,
                 array_size(insert_return_values));
  hash_insert_fake.return_val = BmOK;
  err = add_neighbor_resource(topic, topic_len, &id, port_num);
  ASSERT_EQ(err, BmOK);
  call_count = hash_remove_fake.call_count;
  EXPECT_EQ(call_count, 1);
  call_count = hash_insert_fake.call_count;
  EXPECT_EQ(call_count, 2);
  EXPECT_TRUE(WORKING_ELEMENT.port_mask & port_mask);
  WORKING_ELEMENT.port_mask &= ~port_mask;
}
