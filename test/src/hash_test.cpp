#include "fff.h"
#include "mock_bm_os.h"
#include "gtest/gtest.h"
#include <helpers.hpp>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

DEFINE_FFF_GLOBALS

#include "hash.h"

class hash_test : public ::testing::Test {
protected:
  rnd_gen RND;
  static constexpr uint32_t hash_buf_length = 4096;
  static constexpr uint32_t collision_buf_length = 64;

  typedef struct {
    uint64_t data_field_1;
    uint32_t data_field_2;
    uint16_t data_field_3;
    uint8_t data_field_4;
  } HashData;

  hash_test() {}

  ~hash_test() override {}

  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(hash_test, static_hash) {
  static HashData table[hash_buf_length] = {};
  static uint32_t keys[hash_buf_length] = {};
  static Hash hash = {};

  // Test failure cases
  ASSERT_EQ(
      hash_create_static(NULL, table, keys, sizeof(HashData), hash_buf_length),
      BmEINVAL);
  ASSERT_EQ(
      hash_create_static(&hash, NULL, keys, sizeof(HashData), hash_buf_length),
      BmEINVAL);
  ASSERT_EQ(
      hash_create_static(&hash, table, NULL, sizeof(HashData), hash_buf_length),
      BmEINVAL);
  ASSERT_EQ(hash_create_static(&hash, table, keys, 0, hash_buf_length),
            BmEINVAL);
  ASSERT_EQ(hash_create_static(&hash, table, keys, sizeof(HashData), 0),
            BmEINVAL);

  // Test success
  ASSERT_EQ(
      hash_create_static(&hash, table, keys, sizeof(HashData), hash_buf_length),
      BmOK);
}

TEST_F(hash_test, dynamic_hash) {
  // Test failure cases
  ASSERT_EQ(hash_create(0, hash_buf_length), nullptr);
  ASSERT_EQ(hash_create(sizeof(HashData), 0), nullptr);
  bm_malloc_fail_on_attempt(1);
  ASSERT_EQ(hash_create(sizeof(HashData), hash_buf_length), nullptr);
  bm_malloc_fail_on_attempt(2);
  ASSERT_EQ(hash_create(sizeof(HashData), hash_buf_length), nullptr);
  bm_malloc_fail_on_attempt(3);
  ASSERT_EQ(hash_create(sizeof(HashData), hash_buf_length), nullptr);

  Hash *hash = hash_create(sizeof(HashData), hash_buf_length);
  ASSERT_NE(hash, nullptr);

  ASSERT_EQ(hash_delete(hash), BmOK);
  ASSERT_EQ(hash_delete(NULL), BmEINVAL);
}

TEST_F(hash_test, count) {
  // Test failure cases
  ASSERT_EQ(hash_get_count(NULL), 0);
}

TEST_F(hash_test, hash_insert) {
  Hash *hash = hash_create(sizeof(HashData), hash_buf_length);
  HashData data = {};

  // Test failure cases
  ASSERT_EQ(hash_insert(NULL, 0, &data), BmEINVAL);
  ASSERT_EQ(hash_insert(hash, 0, NULL), BmEINVAL);
  ASSERT_EQ(hash_insert(hash, invalid_key, &data), BmEINVAL);

  hash_delete(hash);
}

TEST_F(hash_test, hash_look_up) {
  Hash *hash = hash_create(sizeof(HashData), hash_buf_length);
  HashData data = {};

  // Test failure cases
  ASSERT_EQ(hash_look_up(NULL, 0, &data), BmEINVAL);
  ASSERT_EQ(hash_look_up(hash, 0, NULL), BmEINVAL);
  ASSERT_EQ(hash_look_up(hash, 0, &data), BmENODATA);
  ASSERT_EQ(hash_look_up(hash, invalid_key, &data), BmEINVAL);

  hash_delete(hash);
}

TEST_F(hash_test, hash_remove) {
  Hash *hash = hash_create(sizeof(HashData), hash_buf_length);

  // Test failure cases
  ASSERT_EQ(hash_remove(NULL, 0), BmEINVAL);
  ASSERT_EQ(hash_remove(hash, invalid_key), BmEINVAL);

  hash_delete(hash);
}

TEST_F(hash_test, hash_get_load) {
  Hash *hash = hash_create(sizeof(HashData), hash_buf_length);

  // Test failure cases
  ASSERT_EQ(hash_get_load(NULL), 0);
  hash->length = 0;
  ASSERT_EQ(hash_get_load(hash), 0);

  hash_delete(hash);
}

TEST_F(hash_test, insert_one) {
  Hash *hash = hash_create(sizeof(HashData), hash_buf_length);
  HashData *insert_data = RND.create_rnd_array<HashData>();
  HashData look_up_data = {};

  ASSERT_EQ(hash_insert(hash, 0, insert_data), BmOK);
  ASSERT_EQ(hash_look_up(hash, 0, &look_up_data), BmOK);

  ASSERT_EQ(0, memcmp(insert_data, &look_up_data, sizeof(HashData)));
  RND.free_rnd_array(insert_data);

  ASSERT_EQ(hash_get_count(hash), 1);

  // Look up hash that does not exist
  ASSERT_EQ(hash_look_up(hash, 1, &look_up_data), BmENODATA);

  hash_delete(hash);
}

TEST_F(hash_test, insert_remove_look_up) {
  Hash *hash = hash_create(sizeof(HashData), hash_buf_length);
  HashData *insert_data = RND.create_rnd_array<HashData>();
  HashData look_up_data = {};

  ASSERT_EQ(hash_insert(hash, 0, insert_data), BmOK);
  ASSERT_EQ(hash_remove(hash, 0), BmOK);
  ASSERT_EQ(hash_look_up(hash, 0, &look_up_data), BmENODATA);

  ASSERT_EQ(hash_get_count(hash), 0);

  RND.free_rnd_array(insert_data);

  hash_delete(hash);
}

TEST_F(hash_test, fill_unique) {
  Hash *hash = hash_create(sizeof(HashData), hash_buf_length);
  HashData *insert_data;
  HashData look_up_data = {};

  // Fill all with unique keys
  for (uint32_t i = 0; i < hash_buf_length; i++) {
    insert_data = RND.create_rnd_array<HashData>();
    ASSERT_EQ(hash_insert(hash, i, insert_data), BmOK);
    ASSERT_EQ(hash_look_up(hash, i, &look_up_data), BmOK);

    ASSERT_EQ(0, memcmp(insert_data, &look_up_data, sizeof(HashData)));

    RND.free_rnd_array(insert_data);
  }
  ASSERT_EQ(hash_get_count(hash), hash_buf_length);
  ASSERT_EQ(hash_get_load(hash), 100);

  insert_data = RND.create_rnd_array<HashData>();
  ASSERT_EQ(hash_insert(hash, 0, insert_data), BmENOSPC);
  RND.free_rnd_array(insert_data);

  // Clear one, insert, validate
  insert_data = RND.create_rnd_array<HashData>();
  ASSERT_EQ(hash_remove(hash, 0), BmOK);
  ASSERT_EQ(hash_insert(hash, hash_buf_length, insert_data), BmOK);
  ASSERT_EQ(hash_look_up(hash, hash_buf_length, &look_up_data), BmOK);
  ASSERT_EQ(hash_remove(hash, hash_buf_length), BmOK);
  ASSERT_EQ(0, memcmp(insert_data, &look_up_data, sizeof(HashData)));
  ASSERT_EQ(0, memcmp(insert_data, &static_cast<HashData *>(hash->table)[0],
                      sizeof(HashData)));
  RND.free_rnd_array(insert_data);

  // Clear all
  for (uint32_t i = 1; i < hash_buf_length; i++) {
    ASSERT_EQ(hash_remove(hash, i), BmOK);
    ASSERT_EQ(hash_look_up(hash, i, &look_up_data), BmENODATA);
  }

  ASSERT_EQ(hash_remove(hash, 0), BmENODATA);

  hash_delete(hash);
}

TEST_F(hash_test, collision) {
  Hash *hash = hash_create(sizeof(HashData), collision_buf_length);
  HashData *insert_data;
  HashData look_up_data = {};
  uint32_t key = 0;

  insert_data = RND.create_rnd_array<HashData>(collision_buf_length);

  ASSERT_EQ(hash_insert(hash, key, &insert_data[0]), BmOK);
  ASSERT_EQ(hash_look_up(hash, key, &look_up_data), BmOK);
  ASSERT_EQ(0, memcmp(&insert_data[0], &look_up_data, sizeof(HashData)));

  key += collision_buf_length;

  ASSERT_EQ(hash_insert(hash, key, &insert_data[1]), BmOK);
  ASSERT_EQ(hash_look_up(hash, key, &look_up_data), BmOK);
  ASSERT_EQ(0, memcmp(&insert_data[1], &look_up_data, sizeof(HashData)));
  ASSERT_EQ(hash_get_count(hash), 2);

  RND.free_rnd_array(insert_data);

  hash_delete(hash);
}

TEST_F(hash_test, collision_chain) {
  static constexpr uint8_t buf_len = 8;
  Hash *hash = hash_create(sizeof(HashData), buf_len);
  HashData *insert_data = RND.create_rnd_array<HashData>(buf_len);
  HashData look_up_data = {};

  // Fill all with collisions
  for (uint32_t i = 0; i < buf_len; i++) {
    ASSERT_EQ(hash_insert(hash, i * buf_len, &insert_data[i]), BmOK);
  }

  // Remove middle elements
  uint32_t remove_idx = 2;
  ASSERT_EQ(hash_remove(hash, remove_idx * buf_len), BmOK);
  remove_idx++;
  ASSERT_EQ(hash_remove(hash, remove_idx * buf_len), BmOK);
  remove_idx++;
  ASSERT_EQ(hash_remove(hash, remove_idx * buf_len), BmOK);
  remove_idx++;

  // Make sure the last elements can be read still with tombstones
  for (uint32_t i = remove_idx; i < buf_len; i++) {
    ASSERT_EQ(hash_look_up(hash, i * buf_len, &look_up_data), BmOK);
    ASSERT_EQ(0, memcmp(&insert_data[i], &look_up_data, sizeof(HashData)));
  }

  ASSERT_EQ(hash_get_count(hash), buf_len - 3);
  RND.free_rnd_array(insert_data);

  hash_delete(hash);
}

TEST_F(hash_test, fill_collision) {
  Hash *hash = hash_create(sizeof(HashData), collision_buf_length);
  HashData *insert_data = RND.create_rnd_array<HashData>(collision_buf_length);
  HashData look_up_data = {};

  // Fill all with collisions
  for (uint32_t i = 0; i < collision_buf_length; i++) {
    ASSERT_EQ(hash_insert(hash, i * collision_buf_length, &insert_data[i]),
              BmOK);
  }

  for (uint32_t i = 0; i < collision_buf_length; i++) {
    ASSERT_EQ(hash_look_up(hash, i * collision_buf_length, &look_up_data),
              BmOK);
    ASSERT_EQ(0, memcmp(&insert_data[i], &look_up_data, sizeof(HashData)));
  }

  ASSERT_EQ(hash_get_count(hash), collision_buf_length);
  ASSERT_EQ(hash_get_load(hash), 100);
  RND.free_rnd_array(insert_data);

  hash_delete(hash);
}

TEST_F(hash_test, tombstone) {
  Hash *hash = hash_create(sizeof(HashData), collision_buf_length);
  HashData *insert_data;
  HashData look_up_data = {};
  uint32_t key = 0;

  insert_data = RND.create_rnd_array<HashData>(collision_buf_length);

  ASSERT_EQ(hash_insert(hash, key, &insert_data[0]), BmOK);
  ASSERT_EQ(hash_look_up(hash, key, &look_up_data), BmOK);
  ASSERT_EQ(0, memcmp(&insert_data[0], &look_up_data, sizeof(HashData)));

  key += collision_buf_length;

  ASSERT_EQ(hash_insert(hash, key, &insert_data[1]), BmOK);
  ASSERT_EQ(hash_look_up(hash, key, &look_up_data), BmOK);
  ASSERT_EQ(0, memcmp(&insert_data[1], &look_up_data, sizeof(HashData)));

  // Remove key, create a tombstone
  ASSERT_EQ(hash_remove(hash, key), BmOK);

  ASSERT_EQ(hash_get_count(hash), 1);

  // Add new value to take tombstone
  key += collision_buf_length;

  ASSERT_EQ(hash_insert(hash, key, &insert_data[2]), BmOK);
  ASSERT_EQ(hash_look_up(hash, key, &look_up_data), BmOK);
  ASSERT_EQ(0, memcmp(&insert_data[2], &look_up_data, sizeof(HashData)));

  // Check that table aligns with data inserted
  ASSERT_EQ(0, memcmp(&insert_data[0], &static_cast<HashData *>(hash->table)[0],
                      sizeof(HashData)));
  ASSERT_EQ(0, memcmp(&insert_data[2], &static_cast<HashData *>(hash->table)[1],
                      sizeof(HashData)));

  ASSERT_EQ(hash_get_count(hash), 2);

  RND.free_rnd_array(insert_data);

  hash_delete(hash);
}

TEST_F(hash_test, wraparound) {
  Hash *hash = hash_create(sizeof(HashData), collision_buf_length);
  HashData *insert_data;
  HashData look_up_data = {};
  uint32_t key = collision_buf_length - 1;

  insert_data = RND.create_rnd_array<HashData>(collision_buf_length);

  ASSERT_EQ(hash_insert(hash, key, &insert_data[0]), BmOK);
  ASSERT_EQ(hash_look_up(hash, key, &look_up_data), BmOK);
  ASSERT_EQ(0, memcmp(&insert_data[0], &look_up_data, sizeof(HashData)));

  key += collision_buf_length;

  // Here at (key * 2) - 1, buffer will wrap around to beginning
  ASSERT_EQ(hash_insert(hash, key, &insert_data[1]), BmOK);
  ASSERT_EQ(hash_look_up(hash, key, &look_up_data), BmOK);
  ASSERT_EQ(0, memcmp(&insert_data[1], &look_up_data, sizeof(HashData)));

  ASSERT_EQ(hash_get_count(hash), 2);

  RND.free_rnd_array(insert_data);

  hash_delete(hash);
}
