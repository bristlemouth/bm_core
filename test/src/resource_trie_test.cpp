#include <gtest/gtest.h>
#include <helpers.hpp>

#include "fff.h"

DEFINE_FFF_GLOBALS

extern "C" {
#include "resource_trie.h"
}

class resource_trie_test : public ::testing::Test {
protected:
  rnd_gen RND;

  resource_trie_test() {}

  ~resource_trie_test() override {}

  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(resource_trie_test, add_elements) {}

TEST_F(resource_trie_test, match_elements) {}

TEST_F(resource_trie_test, match_concrete_simple) {
  ResourceTrieRoot root = {};

  const char *topic = "/sensor/temperature/raw";
  uint32_t resource_id = (uint32_t)RND.rnd_int(max_resource_id, 0);
  uint16_t port_mask = (uint16_t)RND.rnd_int(UINT16_MAX, 1);
  bool local_interest = true;

  BmErr err =
      resource_trie_add(&root, topic, resource_id, port_mask, local_interest);
  ASSERT_EQ(err, BmOK);

  ResourceTrieElement *matches[1] = {};
  ResourceTrieMatchResult result = {
      .matches = matches,
      .capacity = array_size(matches),
      .count = 0,
  };
  err = resource_trie_match(&root, topic, &result);
  ASSERT_EQ(err, BmOK);
  ASSERT_EQ(result.count, 1);
  ResourceTrieElement *match = matches[0];
  EXPECT_EQ(match->local_interest, local_interest);
  EXPECT_EQ(match->resource_id, resource_id);
  EXPECT_EQ(match->port_mask, port_mask);
}

TEST_F(resource_trie_test, match_concrete_split) {
  ResourceTrieRoot root = {};

  const char *topic_1 = "/sensor/temperature/raw";
  const char *topic_2 = "/sensor/temperature";
  uint32_t resource_id[2] = {
      (uint32_t)RND.rnd_int(max_resource_id, 0),
      (uint32_t)RND.rnd_int(max_resource_id, 0),
  };
  uint16_t port_mask[2] = {
      (uint16_t)RND.rnd_int(UINT16_MAX, 1),
      (uint16_t)RND.rnd_int(UINT16_MAX, 1),
  };
  bool local_interest = true;

  BmErr err = resource_trie_add(&root, topic_1, resource_id[0], port_mask[0],
                                local_interest);
  ASSERT_EQ(err, BmOK);
  err = resource_trie_add(&root, topic_2, resource_id[1], port_mask[1],
                          local_interest);
  ASSERT_EQ(err, BmOK);

  ResourceTrieElement *matches[1] = {};
  ResourceTrieMatchResult result = {
      .matches = matches,
      .capacity = array_size(matches),
      .count = 0,
  };
  ResourceTrieElement *match;

  err = resource_trie_match(&root, topic_1, &result);
  ASSERT_EQ(err, BmOK);
  ASSERT_EQ(result.count, 1);
  match = matches[0];
  EXPECT_EQ(match->local_interest, local_interest);
  EXPECT_EQ(match->resource_id, resource_id[0]);
  EXPECT_EQ(match->port_mask, port_mask[0]);

  err = resource_trie_match(&root, topic_2, &result);
  ASSERT_EQ(err, BmOK);
  ASSERT_EQ(result.count, 1);
  match = matches[0];
  EXPECT_EQ(match->local_interest, local_interest);
  EXPECT_EQ(match->resource_id, resource_id[1]);
  EXPECT_EQ(match->port_mask, port_mask[1]);
}

TEST_F(resource_trie_test, match_wildcard_simple) {
  ResourceTrieRoot root = {};

  const char *topic = "/sensor/temperature/raw";
  const char *wildcard = "/sensor/*/raw";
  uint32_t resource_id = RND.rnd_int(max_resource_id, 0);
  uint16_t port_mask = RND.rnd_int(UINT16_MAX, 1);
  bool local_interest = true;

  BmErr err = resource_trie_add(&root, wildcard, resource_id, port_mask,
                                local_interest);
  ASSERT_EQ(err, BmOK);

  ResourceTrieElement *matches[1] = {};
  ResourceTrieMatchResult result = {
      .matches = matches,
      .capacity = array_size(matches),
      .count = 0,
  };
  err = resource_trie_match(&root, topic, &result);
  ASSERT_EQ(err, BmOK);
  ASSERT_EQ(result.count, 1);
  ResourceTrieElement *match = matches[0];
  EXPECT_EQ(match->local_interest, local_interest);
  EXPECT_EQ(match->resource_id, resource_id);
  EXPECT_EQ(match->port_mask, port_mask);
}

TEST_F(resource_trie_test, wildcard_concrete_first_update_outputs) {
  ResourceTrieRoot root = {};

  const char *concrete_topic = "/sensor/temperature/raw";
  const char *wildcard_topic = "/sensor/*/raw";

  uint32_t concrete_id = 100;
  uint16_t concrete_port = 0x0001;
  uint32_t wildcard_id = 200;
  uint16_t wildcard_port = 0x0002;

  BmErr err = resource_trie_add(&root, concrete_topic, concrete_id,
                                concrete_port, false);
  ASSERT_EQ(err, BmOK);

  // Adding a wildcard that overlaps an existing concrete element updates
  // the concrete element's port_mask (OR'd) and sets its local_interest = true
  err = resource_trie_add(&root, wildcard_topic, wildcard_id, wildcard_port,
                          true);
  ASSERT_EQ(err, BmOK);

  ResourceTrieElement *matches[2] = {};
  ResourceTrieMatchResult result = {
      .matches = matches,
      .capacity = array_size(matches),
      .count = 0,
  };

  // Querying the concrete topic should return both the wildcard and concrete
  // entries
  err = resource_trie_match(&root, concrete_topic, &result);
  ASSERT_EQ(err, BmOK);
  ASSERT_EQ(result.count, 2);

  // First match: the wildcard entry (unchanged)
  EXPECT_EQ(matches[0]->resource_id, wildcard_id);
  EXPECT_EQ(matches[0]->port_mask, wildcard_port);
  EXPECT_EQ(matches[0]->local_interest, true);

  // Second match: the concrete entry, mutated by the overlapping wildcard add
  EXPECT_EQ(matches[1]->resource_id, concrete_id);
  EXPECT_EQ(matches[1]->port_mask, concrete_port | wildcard_port);
  EXPECT_EQ(matches[1]->local_interest, true); // promoted from false

  // Querying the wildcard topic should produce identical results
  err = resource_trie_match(&root, wildcard_topic, &result);
  ASSERT_EQ(err, BmOK);
  ASSERT_EQ(result.count, 2);
  EXPECT_EQ(matches[0]->resource_id, wildcard_id);
  EXPECT_EQ(matches[0]->port_mask, wildcard_port);
  EXPECT_EQ(matches[0]->local_interest, true);
  EXPECT_EQ(matches[1]->resource_id, concrete_id);
  EXPECT_EQ(matches[1]->port_mask, concrete_port | wildcard_port);
  EXPECT_EQ(matches[1]->local_interest, true);
}

TEST_F(resource_trie_test, wildcard_first_update_outputs) {
  ResourceTrieRoot root = {};

  const char *concrete_topic = "/sensor/temperature/raw";
  const char *wildcard_topic = "/sensor/*/raw";

  uint32_t concrete_id = 100;
  uint16_t concrete_port = 0;
  uint32_t wildcard_id = 200;
  uint16_t wildcard_port = 0x0002;

  BmErr err = resource_trie_add(&root, wildcard_topic, wildcard_id,
                                wildcard_port, true);
  ASSERT_EQ(err, BmOK);

  // Adding a concrete that overlaps an existing wildcard element updates
  // the concrete element's port_mask (OR'd) and sets its local_interest = true
  err = resource_trie_add(&root, concrete_topic, concrete_id, concrete_port,
                          false);
  ASSERT_EQ(err, BmOK);

  ResourceTrieElement *matches[2] = {};
  ResourceTrieMatchResult result = {
      .matches = matches,
      .capacity = array_size(matches),
      .count = 0,
  };

  // Querying the concrete topic should return both the wildcard and concrete
  // entries
  err = resource_trie_match(&root, concrete_topic, &result);
  ASSERT_EQ(err, BmOK);
  ASSERT_EQ(result.count, 2);

  // First match: the concrete entry mutated by the overlapping wildcard
  EXPECT_EQ(matches[0]->resource_id, concrete_id);
  EXPECT_EQ(matches[0]->port_mask, wildcard_port);
  EXPECT_EQ(matches[0]->local_interest, true);

  // Second match: the wildcard entry (unchanged)
  EXPECT_EQ(matches[1]->resource_id, wildcard_id);
  EXPECT_EQ(matches[1]->port_mask, wildcard_port);
  EXPECT_EQ(matches[1]->local_interest, true); // promoted from false
}
