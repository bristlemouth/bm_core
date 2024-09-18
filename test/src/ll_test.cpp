#include "gtest/gtest.h"
#include <helpers.hpp>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "ll.h"
#ifdef __cplusplus
}
#endif

#define min_items 1024
#define max_items 8192
#define traverse_cb_val UINT16_MAX
#define traverse_count 10
#define traverse_write_data 0xBF

typedef int (*LLTestCb_t)(LL *ll, LLItem item);
static LLItem *ITEMS;
static uint8_t *ORIGIN;
static size_t NCOUNT;
static size_t BSIZE;
static uint32_t NOREACH;
static rnd_gen RND;

class ll_test : public ::testing::Test {
protected:
  ll_test() {}

  ~ll_test() override {}

  void SetUp() override {
    BSIZE = 0;
    NCOUNT = 0;
    if (ITEMS) {
      ITEMS = NULL;
    }
    if (ORIGIN) {
      ORIGIN = NULL;
    }
  }

  void TearDown() override {
    if (ITEMS) {
      free(ITEMS);
      ITEMS = NULL;
    }
    if (ORIGIN) {
      free(ORIGIN);
      ORIGIN = NULL;
    }
  }

  LL create_test_helper(LLTestCb_t cb, bool dynamic) {
    NCOUNT = RND.rnd_int(max_items, min_items);
    BSIZE = RND.rnd_int(UINT8_MAX, UINT8_MAX / 2);
    ITEMS = (LLItem *)calloc(NCOUNT, sizeof(LLItem));
    LL ll = {NULL, NULL, NULL};
    LLItem *dyn_track = NULL;
    LLItem *dyn_item = NULL;
    uint8_t *data = (uint8_t *)calloc(NCOUNT * BSIZE, sizeof(uint8_t));
    ORIGIN = data;
    uint32_t seed = time(NULL);
    rand_sequence_unique rsu(seed, seed + 1);

    // Add random data to linked list item pointer and push to instance
    for (size_t i = 0; i < NCOUNT; i++) {
      RND.rnd_array(data, BSIZE);
      if (!dynamic) {
        EXPECT_EQ(ll_create_item_static(&ITEMS[i], data, rsu.next()), BmOK);
        EXPECT_EQ(ll_item_add(&ll, &ITEMS[i]), BmOK);
      } else {
        dyn_item = ll_create_item(dyn_item, data, rsu.next());
        if (!dyn_track) {
          dyn_track = dyn_item;
        }
        EXPECT_NE(dyn_item, nullptr);
        EXPECT_EQ(ll_item_add(&ll, dyn_item), BmOK);
        memcpy(&ITEMS[i], dyn_item, sizeof(LLItem));
      }
      data += BSIZE;
    }

    // Set a item id that is not reachable
    NOREACH = rsu.next();

    // Run callback in random order (Fisher-Yates shuffle)
    // with a copy of Linked List to avoid changing original addresses
    if (cb) {
      LLItem *copy = (LLItem *)calloc(NCOUNT, sizeof(LLItem));
      memcpy(copy, ITEMS, sizeof(LLItem) * NCOUNT);
      for (size_t i = 0; i < NCOUNT - 1; i++) {
        size_t j = RND.rnd_int(NCOUNT - 1, i);
        LLItem temp;
        temp = copy[i];
        copy[i] = copy[j];
        copy[j] = temp;
        EXPECT_EQ(cb(&ll, copy[i]), BmOK);
      }
      free(copy);
    }

    return ll;
  }

  void ll_clean_up_helper(LL *ll) {
    for (size_t i = 0; i < NCOUNT; i++) {
      ll_remove(ll, ITEMS[i].id);
    }
  }
};

TEST_F(ll_test, add) {
  LL ll = create_test_helper(NULL, false);
  LLItem item;
  // Test improper use case
  EXPECT_NE(ll_item_add(NULL, NULL), 0);
  EXPECT_NE(ll_item_add(&ll, NULL), 0);
  EXPECT_NE(ll_item_add(NULL, &item), 0);
  ll_clean_up_helper(&ll);

  // Test dynamic allocation
  ll = create_test_helper(NULL, true);
  ll_clean_up_helper(&ll);
}

static int ll_get_test_helper(LL *ll, LLItem item) {
  int ret = 0;
  uint8_t *cmp = NULL;
  EXPECT_EQ(ll_get_item(ll, item.id, (void **)&cmp), 0);
  EXPECT_NE(cmp, nullptr);
  if (cmp) {
    EXPECT_EQ(memcmp(cmp, item.data, BSIZE), 0);
  }
  return ret;
}

TEST_F(ll_test, get) {
  LL ll = create_test_helper(ll_get_test_helper, false);
  void *cmp = NULL;
  // Test improper use case
  EXPECT_NE(ll_get_item(NULL, 0, &cmp), BmOK);
  EXPECT_NE(ll_get_item(&ll, 0, NULL), BmOK);
  EXPECT_NE(ll_get_item(&ll, NOREACH, &cmp), BmOK);
  ll_clean_up_helper(&ll);

  // Test dynamic allocation
  ll = create_test_helper(ll_get_test_helper, true);
  // Test improper use case
  EXPECT_NE(ll_get_item(&ll, NOREACH, &cmp), BmOK);
  ll_clean_up_helper(&ll);
}

static int ll_remove_test_helper(LL *ll, LLItem item) {
  int ret = 0;
  uint8_t *data = NULL;
  EXPECT_EQ(ll_remove(ll, item.id), BmOK);
  // Test we can no longer find this item
  EXPECT_NE(ll_remove(ll, item.id), BmOK);
  EXPECT_NE(ll_get_item(ll, item.id, (void **)&data), BmOK);
  return ret;
}

TEST_F(ll_test, remove) {
  LL ll = create_test_helper(ll_remove_test_helper, false);
  // Test improper use case
  ASSERT_NE(ll_remove(NULL, 0), BmOK);
  ASSERT_NE(ll_remove(&ll, NOREACH), BmOK);
  ll_clean_up_helper(&ll);

  // Test dynamic allocation
  ll = create_test_helper(ll_remove_test_helper, true);
  ASSERT_NE(ll_remove(&ll, NOREACH), BmOK);
  ll_clean_up_helper(&ll);
}

static BmErr ll_traverse_test_helper(void *data, void *arg) {
  static size_t c = 0;
  uint8_t *cmp = (uint8_t *)data;
  EXPECT_EQ(*(uint32_t *)arg, traverse_cb_val);
  EXPECT_EQ(memcmp(cmp, ITEMS[c++ % NCOUNT].data, BSIZE), BmOK);
  memset(cmp, traverse_write_data, BSIZE);
  return BmOK;
}

TEST_F(ll_test, traverse) {
  LL ll = create_test_helper(NULL, false);
  uint32_t arg = traverse_cb_val;
  for (size_t i = 0; i < traverse_count; i++) {
    ll_traverse(&ll, ll_traverse_test_helper, &arg);
  }
  // Test accessed variable in callback is properly manipulated
  for (size_t i = 0; i < NCOUNT * BSIZE; i++) {
    EXPECT_EQ(ORIGIN[i], traverse_write_data);
  }
  // Test improper use cases
  ASSERT_NE(ll_traverse(NULL, NULL, &arg), BmOK);
  ASSERT_NE(ll_traverse(&ll, NULL, &arg), BmOK);
  ASSERT_NE(ll_traverse(NULL, ll_traverse_test_helper, &arg), BmOK);

  ll_clean_up_helper(&ll);
}
