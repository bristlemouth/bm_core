#include "fff.h"
#include "gtest/gtest.h"
#include <helpers.hpp>

DEFINE_FFF_GLOBALS;

extern "C" {
#include "cb_queue.h"
}

static uint16_t CB_COUNT = 0;
static uint32_t TEST_ARG = 0;

static BmErr cb_ok(const void *arg) {
  BmErr err = BmOK;
  uint32_t cmp_val = *(uint32_t *)arg;
  CB_COUNT++;
  EXPECT_EQ(cmp_val, TEST_ARG);
  return err;
}

static BmErr cb_fail(const void *arg) { return BmEBADMSG; }

TEST(CbQueue, enqueue_dequeue_ok) {
  BmCbQueue q = {};
  rnd_gen RND;
  uint16_t queue_size = (uint16_t)RND.rnd_int(UINT16_MAX, 1000);

  // Test NULL cbs
  for (uint32_t i = 0; i < queue_size; i++) {
    ASSERT_EQ(queue_cb_enqueue(&q, NULL), BmOK);
  }
  for (uint32_t i = 0; i < queue_size; i++) {
    ASSERT_EQ(queue_cb_dequeue(&q, &TEST_ARG, true), BmOK);
  }
  ASSERT_EQ(CB_COUNT, 0);

  // Test successful cbs
  for (uint32_t i = 0; i < queue_size; i++) {
    ASSERT_EQ(queue_cb_enqueue(&q, cb_ok), BmOK);
  }
  for (uint32_t i = 0; i < queue_size; i++) {
    TEST_ARG = (uint32_t)RND.rnd_int(UINT32_MAX, 0);
    ASSERT_EQ(queue_cb_dequeue(&q, &TEST_ARG, true), BmOK);
  }
  ASSERT_EQ(CB_COUNT, queue_size);

  // Test not invoking callbacks
  for (uint32_t i = 0; i < queue_size; i++) {
    ASSERT_EQ(queue_cb_enqueue(&q, cb_ok), BmOK);
  }
  for (uint32_t i = 0; i < queue_size; i++) {
    TEST_ARG = (uint32_t)RND.rnd_int(UINT32_MAX, 0);
    ASSERT_EQ(queue_cb_dequeue(&q, &TEST_ARG, false), BmOK);
  }
  ASSERT_EQ(CB_COUNT, queue_size);

  // Cleanup
  CB_COUNT = 0;
  TEST_ARG = 0;
}

TEST(CbQueue, enqueue_dequeue_fail) {
  BmCbQueue q = {};
  rnd_gen RND;
  uint16_t queue_size = (uint16_t)RND.rnd_int(UINT16_MAX, 1000);

  // Test dequeueing when there is nothing in the queue
  for (uint32_t i = 0; i < queue_size; i++) {
    ASSERT_EQ(queue_cb_enqueue(&q, NULL), BmOK);
  }
  for (uint32_t i = 0; i < queue_size; i++) {
    ASSERT_EQ(queue_cb_dequeue(&q, &TEST_ARG, false), BmOK);
  }
  for (uint32_t i = 0; i < queue_size; i++) {
    ASSERT_EQ(queue_cb_dequeue(&q, &TEST_ARG, false), BmECANCELED);
  }

  // Test callback failure
  ASSERT_EQ(queue_cb_enqueue(&q, cb_fail), BmOK);
  ASSERT_EQ(queue_cb_dequeue(&q, NULL, true), BmEBADMSG);
}
