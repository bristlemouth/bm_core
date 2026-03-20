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

TEST_F(resource_trie_test, match_exact_simple) {
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
  EXPECT_EQ(result.count, 1);
  ResourceTrieElement *match = matches[0];
  EXPECT_EQ(match->local_interest, local_interest);
  EXPECT_EQ(match->resource_id, resource_id);
  EXPECT_EQ(match->port_mask, port_mask);
}

TEST_F(resource_trie_test, match_exact_split) {
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
  EXPECT_EQ(result.count, 1);
  match = matches[0];
  EXPECT_EQ(match->local_interest, local_interest);
  EXPECT_EQ(match->resource_id, resource_id[0]);
  EXPECT_EQ(match->port_mask, port_mask[0]);

  err = resource_trie_match(&root, topic_2, &result);
  ASSERT_EQ(err, BmOK);
  EXPECT_EQ(result.count, 1);
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

  ResourceTrieElement *matches[2] = {};
  ResourceTrieMatchResult result = {
      .matches = matches,
      .capacity = array_size(matches),
      .count = 0,
  };
  err = resource_trie_match(&root, topic, &result);
  ASSERT_EQ(err, BmOK);
  EXPECT_EQ(result.count, 1);
  ResourceTrieElement *match = matches[0];
  EXPECT_EQ(match->local_interest, local_interest);
  EXPECT_EQ(match->resource_id, resource_id);
  EXPECT_EQ(match->port_mask, port_mask);
}
