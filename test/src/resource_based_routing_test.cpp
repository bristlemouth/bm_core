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
#include <string.h>

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

    bm_l2_get_port_count_fake.return_val = 16;
  }
  void TearDown() override {}

  static BmErr fake_match_exact_success(ResourceTrieRoot *root,
                                        const char *topic, BmTopicLength len) {
    (void)topic;
    (void)len;
    root->result.count = 1;
    root->result.matches[0] = &WORKING_ELEMENT;

    return BmOK;
  }

  static BmErr fake_match_exact_failure(ResourceTrieRoot *root,
                                        const char *topic, BmTopicLength len) {
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

  static BmErr fake_add_fail(ResourceTrieRoot *root, const char *topic,
                             BmTopicLength len, ResourceId id,
                             uint16_t port_mask, bool local_interest) {
    (void)topic;
    (void)len;
    root->result.count = 0;
    root->result.matches[0] = NULL;

    return BmENOMEM;
  }

  static BmErr fake_look_up_success(Hash *hash, uint32_t key, void *data) {
    (void)hash;
    (void)key;

    *(ResourceTrieElement **)data = &WORKING_ELEMENT;

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
  resource_trie_match_exact_fake.custom_fake = fake_match_exact_success;
  err = add_local_resource(topic, topic_len, &id);
  ASSERT_EQ(err, BmOK);
  call_count = resource_trie_add_fake.call_count;
  EXPECT_EQ(call_count, 0);
  EXPECT_EQ(id, WORKING_ELEMENT.resource_id);

  // Test failure paths
  RESET_FAKE(resource_trie_match_exact);
  RESET_FAKE(resource_trie_add);
  resource_trie_match_exact_fake.custom_fake = fake_match_exact_failure;
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
  uint16_t port_mask = 1 << (port_num - 1);

  hash_look_up_fake.return_val = BmENODATA;

  // Test adding path
  RESET_FAKE(resource_trie_match_exact);
  RESET_FAKE(resource_trie_add);
  RESET_FAKE(hash_insert);
  resource_trie_match_exact_fake.return_val = BmOK;
  resource_trie_add_fake.custom_fake = fake_add_success;
  hash_insert_fake.return_val = BmOK;
  BmErr err = add_neighbor_resource(topic, topic_len, &id, port_num, true);
  ASSERT_EQ(err, BmOK);
  uint8_t call_count = resource_trie_add_fake.call_count;
  EXPECT_EQ(call_count, 1);
  call_count = hash_insert_fake.call_count;
  EXPECT_EQ(call_count, 1);
  EXPECT_TRUE(WORKING_ELEMENT.port_mask & port_mask);
  WORKING_ELEMENT.port_mask &= ~port_mask;

  // Test adding and not updating mask
  err = add_neighbor_resource(topic, topic_len, &id, port_num, false);
  ASSERT_EQ(err, BmOK);
  EXPECT_FALSE(WORKING_ELEMENT.port_mask & port_mask);

  // Test trie existing path
  RESET_FAKE(resource_trie_match_exact);
  RESET_FAKE(resource_trie_add);
  RESET_FAKE(hash_insert);
  resource_trie_match_exact_fake.custom_fake = fake_match_exact_success;
  hash_insert_fake.return_val = BmOK;
  err = add_neighbor_resource(topic, topic_len, &id, port_num, true);
  ASSERT_EQ(err, BmOK);
  call_count = resource_trie_add_fake.call_count;
  EXPECT_EQ(call_count, 0);
  call_count = hash_insert_fake.call_count;
  EXPECT_EQ(call_count, 1);
  EXPECT_TRUE(WORKING_ELEMENT.port_mask & port_mask);
  WORKING_ELEMENT.port_mask &= ~port_mask;

  // Test trie existing path and not updating mask
  err = add_neighbor_resource(topic, topic_len, &id, port_num, false);
  ASSERT_EQ(err, BmOK);
  EXPECT_FALSE(WORKING_ELEMENT.port_mask & port_mask);

  // Test hash existing path
  RESET_FAKE(hash_insert);
  RESET_FAKE(hash_remove);
  hash_look_up_fake.return_val = BmOK;
  hash_remove_fake.return_val = BmOK;
  hash_insert_fake.return_val = BmOK;
  err = add_neighbor_resource(topic, topic_len, &id, port_num, true);
  ASSERT_EQ(err, BmOK);
  call_count = hash_remove_fake.call_count;
  EXPECT_EQ(call_count, 1);
  call_count = hash_insert_fake.call_count;
  EXPECT_EQ(call_count, 1);
  EXPECT_TRUE(WORKING_ELEMENT.port_mask & port_mask);
  WORKING_ELEMENT.port_mask &= ~port_mask;

  // Test failure to remove
  RESET_FAKE(hash_insert);
  RESET_FAKE(hash_remove);
  hash_remove_fake.return_val = BmEINVAL;
  err = add_neighbor_resource(topic, topic_len, &id, port_num, true);
  ASSERT_EQ(err, BmEINVAL);
  call_count = hash_insert_fake.call_count;
  EXPECT_EQ(call_count, 0);
  EXPECT_FALSE(WORKING_ELEMENT.port_mask & port_mask);

  // Test failure to insert
  RESET_FAKE(hash_insert);
  RESET_FAKE(hash_remove);
  id = neighbor_id;
  hash_remove_fake.return_val = BmOK;
  hash_insert_fake.return_val = BmEBADMSG;
  err = add_neighbor_resource(topic, topic_len, &id, port_num, true);
  ASSERT_EQ(err, BmEBADMSG);
  call_count = hash_insert_fake.call_count;
  EXPECT_EQ(call_count, 1);
  EXPECT_EQ(id, neighbor_id);
  EXPECT_FALSE(WORKING_ELEMENT.port_mask & port_mask);

  // Test failure to match
  RESET_FAKE(resource_trie_match_exact);
  resource_trie_match_exact_fake.custom_fake = fake_match_exact_failure;
  err = add_neighbor_resource(topic, topic_len, &id, port_num, true);
  ASSERT_NE(err, BmOK);

  // Test failure to add to trie
  RESET_FAKE(resource_trie_match_exact);
  RESET_FAKE(resource_trie_add);
  resource_trie_match_exact_fake.return_val = BmOK;
  resource_trie_add_fake.custom_fake = fake_add_fail;
  err = add_neighbor_resource(topic, topic_len, &id, port_num, true);
  ASSERT_NE(err, BmOK);
  call_count = resource_trie_add_fake.call_count;
  EXPECT_EQ(call_count, 1);
  EXPECT_FALSE(WORKING_ELEMENT.port_mask & port_mask);

  // Test invalid arguments
  err = add_neighbor_resource(NULL, topic_len, &id, port_num, true);
  ASSERT_EQ(err, BmEINVAL);
  err = add_neighbor_resource(topic, 0, &id, port_num, true);
  ASSERT_EQ(err, BmEINVAL);
  err = add_neighbor_resource(topic, topic_len, NULL, port_num, true);
  ASSERT_EQ(err, BmEINVAL);
  err = add_neighbor_resource(topic, topic_len, &id, UINT8_MAX, true);
  ASSERT_EQ(err, BmEINVAL);
}

TEST_F(resource_based_routing_test, remove_local_resource) {
  static constexpr char topic[] = "test/topic/1";
  const BmTopicLength topic_len = strlen(topic);

  // Test could not find match
  RESET_FAKE(resource_trie_match_exact);
  resource_trie_match_exact_fake.return_val = BmOK;
  BmErr err = remove_local_resource(topic, topic_len);
  ASSERT_EQ(err, BmENODATA);

  // Test found match success no erase case
  RESET_FAKE(resource_trie_match_exact);
  RESET_FAKE(resource_trie_remove);
  resource_trie_match_exact_fake.custom_fake = fake_match_exact_success;
  err = remove_local_resource(topic, topic_len);
  ASSERT_EQ(err, BmOK);
  uint8_t call_count = resource_trie_remove_fake.call_count;
  EXPECT_EQ(call_count, 0);

  // Test found match success erase case
  resource_trie_remove_fake.return_val = BmOK;
  WORKING_ELEMENT.port_mask = 0; // must be 0 to erase
  err = remove_local_resource(topic, topic_len);
  ASSERT_EQ(err, BmOK);
  call_count = resource_trie_remove_fake.call_count;
  EXPECT_EQ(call_count, 1);

  // Test found match failed to remove case
  RESET_FAKE(resource_trie_remove);
  resource_trie_remove_fake.return_val = BmEBADMSG;
  err = remove_local_resource(topic, topic_len);
  ASSERT_EQ(err, BmEBADMSG);
  call_count = resource_trie_remove_fake.call_count;
  EXPECT_EQ(call_count, 1);

  // Test match failure case
  RESET_FAKE(resource_trie_match_exact);
  resource_trie_match_exact_fake.custom_fake = fake_match_exact_failure;
  err = remove_local_resource(topic, topic_len);
  ASSERT_NE(err, BmOK);

  // Test input failures
  err = remove_local_resource(NULL, topic_len);
  ASSERT_EQ(err, BmEINVAL);
  err = remove_local_resource(topic, 0);
  ASSERT_EQ(err, BmEINVAL);
}

TEST_F(resource_based_routing_test, remove_neighbor_resource) {
  static constexpr char topic[] = "test/topic/1";
  const BmTopicLength topic_len = strlen(topic);
  const uint8_t port_num = RND.rnd_int(16, 1);

  // Test could not find match
  RESET_FAKE(resource_trie_match_exact);
  resource_trie_match_exact_fake.return_val = BmOK;
  BmErr err = remove_neighbor_resource(topic, topic_len, port_num);
  ASSERT_EQ(err, BmENODATA);

  // Test found match success, remove from table but not trie
  RESET_FAKE(resource_trie_match_exact);
  RESET_FAKE(resource_trie_remove);
  RESET_FAKE(hash_remove);
  resource_trie_match_exact_fake.custom_fake = fake_match_exact_success;
  hash_remove_fake.return_val = BmOK;
  err = remove_neighbor_resource(topic, topic_len, port_num);
  ASSERT_EQ(err, BmOK);
  uint8_t call_count = resource_trie_remove_fake.call_count;
  EXPECT_EQ(call_count, 0);
  call_count = hash_remove_fake.call_count;
  EXPECT_EQ(call_count, 1);

  // Test found match success, remove from table failure
  hash_remove_fake.return_val = BmEBADMSG;
  err = remove_neighbor_resource(topic, topic_len, port_num);
  ASSERT_EQ(err, BmEBADMSG);

  // Test found match success, remove from table and trie
  RESET_FAKE(resource_trie_remove);
  RESET_FAKE(hash_remove);
  WORKING_ELEMENT.local_interest = 0;              // must be 0 to erase
  WORKING_ELEMENT.port_mask = 1 << (port_num - 1); // must be 0 to erase
  hash_remove_fake.return_val = BmOK;
  resource_trie_remove_fake.return_val = BmOK;
  err = remove_neighbor_resource(topic, topic_len, port_num);
  ASSERT_EQ(err, BmOK);
  call_count = resource_trie_remove_fake.call_count;
  EXPECT_EQ(call_count, 1);

  // Test match failure
  RESET_FAKE(resource_trie_match_exact);
  resource_trie_match_exact_fake.custom_fake = fake_match_exact_failure;
  err = remove_neighbor_resource(topic, topic_len, port_num);
  ASSERT_NE(err, BmOK);

  // Test input failures
  err = remove_neighbor_resource(NULL, topic_len, port_num);
  ASSERT_EQ(err, BmEINVAL);
  err = remove_neighbor_resource(topic, 0, port_num);
  ASSERT_EQ(err, BmEINVAL);
  err = remove_neighbor_resource(topic, topic_len, UINT8_MAX);
  ASSERT_EQ(err, BmEINVAL);
  err = remove_neighbor_resource(topic, topic_len, 0);
  ASSERT_EQ(err, BmEINVAL);
}

TEST_F(resource_based_routing_test, get_forward_port_mask) {
  const uint8_t port_num = RND.rnd_int(16, 1);
  const ResourceId neighbor_id = RND.rnd_int(0xFFFFF, 1);
  ResourceId id = neighbor_id;

  uint16_t forward_mask;
  bool local_interest;
  ResourceOptions opts;

  // Successful lookup
  RESET_FAKE(hash_look_up);
  hash_look_up_fake.custom_fake = fake_look_up_success;
  BmErr err = get_forward_port_mask(&id, port_num, &forward_mask,
                                    &local_interest, opts);
  ASSERT_EQ(err, BmOK);
  bool cmp_local_interest = (bool)WORKING_ELEMENT.local_interest |
                            (bool)WORKING_ELEMENT.wildcard_interest;
  EXPECT_EQ(local_interest, cmp_local_interest);
  uint16_t cmp_forward_mask =
      (WORKING_ELEMENT.port_mask | WORKING_ELEMENT.wildcard_port_mask) &
      ~(1 << (port_num - 1));
  EXPECT_EQ(forward_mask, cmp_forward_mask);

  // Test failure to lookup
  RESET_FAKE(hash_look_up);
  hash_look_up_fake.return_val = BmENODATA;
  err = get_forward_port_mask(&id, port_num, &forward_mask, &local_interest,
                              opts);
  ASSERT_EQ(err, BmENODATA);

  // Test invalid inputs
  err = get_forward_port_mask(NULL, port_num, &forward_mask, &local_interest,
                              opts);
  ASSERT_EQ(err, BmEINVAL);
  err = get_forward_port_mask(&id, 0, &forward_mask, &local_interest, opts);
  ASSERT_EQ(err, BmEINVAL);
  err = get_forward_port_mask(&id, UINT8_MAX, &forward_mask, &local_interest,
                              opts);
  ASSERT_EQ(err, BmEINVAL);
  err = get_forward_port_mask(&id, port_num, NULL, &local_interest, opts);
  ASSERT_EQ(err, BmEINVAL);
  err = get_forward_port_mask(&id, port_num, &forward_mask, NULL, opts);
  ASSERT_EQ(err, BmEINVAL);
}

TEST_F(resource_based_routing_test, get_topic_element) {
  static constexpr char topic[] = "test/topic/1";
  const BmTopicLength topic_len = strlen(topic);
  ResourceTrieElement element;

  // Test no match case
  RESET_FAKE(resource_trie_match_exact);
  resource_trie_match_exact_fake.return_val = BmOK;
  BmErr err = get_topic_element(topic, topic_len, &element);
  ASSERT_EQ(err, BmENODATA);

  // Test success case
  RESET_FAKE(resource_trie_match_exact);
  resource_trie_match_exact_fake.custom_fake = fake_match_exact_success;
  err = get_topic_element(topic, topic_len, &element);
  ASSERT_EQ(err, BmOK);
  int cmp = memcmp(&element, &WORKING_ELEMENT, sizeof(ResourceTrieElement));
  ASSERT_EQ(cmp, 0);

  // Test match failure
  RESET_FAKE(resource_trie_match_exact);
  resource_trie_match_exact_fake.custom_fake = fake_match_exact_failure;
  err = get_topic_element(topic, topic_len, &element);
  ASSERT_NE(err, BmOK);

  // Test invalid inputs
  err = get_topic_element(NULL, topic_len, &element);
  ASSERT_EQ(err, BmEINVAL);
  err = get_topic_element(topic, 0, &element);
  ASSERT_EQ(err, BmEINVAL);
  err = get_topic_element(topic, topic_len, NULL);
  ASSERT_EQ(err, BmEINVAL);
}
