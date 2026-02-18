#include "bm_os.h"
#include "fff.h"
#include "gtest/gtest.h"
#include <helpers.hpp>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

DEFINE_FFF_GLOBALS

#include "q.h"

class queue_test : public ::testing::Test {
protected:
  rnd_gen RND;
  static constexpr uint32_t queue_buf_size = 4096;

  queue_test() {}

  ~queue_test() override {}

  void SetUp() override {}

  void TearDown() override {}

  void enqueue_dequeue_check(Q *queue) {
    // Ensure there is enough space for items in the queue, make queue items small
    constexpr size_t num_queue_items = 10;
    constexpr size_t queue_item_size = (queue_buf_size / num_queue_items) -
                                       (queue_buf_size / (num_queue_items * 2));
    uint8_t queue_item_bufs[num_queue_items][queue_item_size] = {};

    for (size_t i = 0; i < num_queue_items; i++) {
      RND.rnd_array(queue_item_bufs[i], queue_item_size);
      ASSERT_EQ(q_enqueue(queue, queue_item_bufs[i], queue_item_size), BmOK);
    }

    // Ensure that failure occurs if queue item is larger than space available
    constexpr size_t queue_big_buf_size =
        queue_buf_size - (num_queue_items * queue_item_size);
    uint8_t queue_big_buf[queue_big_buf_size] = {};
    RND.rnd_array(queue_big_buf, queue_big_buf_size);
    EXPECT_NE(q_enqueue(queue, queue_big_buf, queue_big_buf_size), BmOK);

    // Dequeue buffer and ensure it equals what was enqueued
    uint8_t comp_buf[queue_item_size] = {0};

    for (size_t i = 0; i < num_queue_items; i++) {
      ASSERT_EQ(q_dequeue(queue, comp_buf, queue_item_size), BmOK);
      size_t res = memcmp(comp_buf, queue_item_bufs[i], queue_item_size);
      ASSERT_EQ(res, 0);
    }
  }
};

TEST_F(queue_test, static_queue) {
  static uint8_t buf[queue_buf_size] = {};
  static Q queue = {};

  // Test failure cases
  ASSERT_EQ(q_create_static(NULL, buf, queue_buf_size), BmEINVAL);
  ASSERT_EQ(q_create_static(&queue, NULL, queue_buf_size), BmEINVAL);
  ASSERT_EQ(q_create_static(&queue, buf, 0), BmEINVAL);

  // Test pass case
  ASSERT_EQ(q_create_static(&queue, buf, queue_buf_size), BmOK);

  enqueue_dequeue_check(&queue);
}

TEST_F(queue_test, dynamic_queue) {

  // Test failure cases
  ASSERT_EQ(q_create(0), nullptr);

  // Test pass case
  Q *queue = q_create(queue_buf_size);

  enqueue_dequeue_check(queue);

  q_delete(queue);
}

TEST_F(queue_test, enqueue) {
  Q *queue = q_create(queue_buf_size);

  size_t queue_elements = 0;
  size_t max_queue_filled = queue_buf_size / 2;

  // Test failure cases
  uint8_t *fail_buf = (uint8_t *)bm_malloc(max_queue_filled);
  EXPECT_EQ(q_enqueue(NULL, fail_buf, max_queue_filled), BmEINVAL);
  EXPECT_EQ(q_enqueue(queue, NULL, max_queue_filled), BmEINVAL);
  EXPECT_EQ(q_enqueue(queue, fail_buf, 0), BmEINVAL);
  bm_free(fail_buf);

  // Test enqueueing of different sizes up until about max_queue_filled
  for (size_t i = 0; i < max_queue_filled;) {
    uint8_t buf_size = RND.rnd_int(UINT8_MAX, 1);

    // Ensure that head does not past max_queue_filled
    buf_size = queue->head + buf_size < max_queue_filled
                   ? buf_size
                   : max_queue_filled - queue->head;

    // Enqueue buffer
    uint8_t *buf = (uint8_t *)bm_malloc(buf_size);
    ASSERT_EQ(q_enqueue(queue, buf, buf_size), BmOK);
    bm_free(buf);

    i += buf_size;
    queue_elements++;
  }

  // Test enqueueing wrap around:
  //   - First dequeue the buffer so tail != 0
  //   - Then write about 3/4 of the size of the queue to wrap around 0 index
  //   - Assert that head < tail
  uint8_t *dequeue_buf = (uint8_t *)bm_malloc(queue_buf_size);
  for (size_t i = 0; i < queue_elements; i++) {
    ASSERT_EQ(q_dequeue(queue, dequeue_buf, queue_buf_size), BmOK);
  }
  bm_free(dequeue_buf);
  EXPECT_EQ(queue->count, 0);

  max_queue_filled = 3 * queue_buf_size / 4;
  for (size_t i = 0; i < max_queue_filled;) {
    uint8_t buf_size = RND.rnd_int(UINT8_MAX, 1);

    // Ensure queue head does not past tail on wrap around
    uint32_t check_size = queue->head > queue->tail ? i : queue->head;
    buf_size = bm_min(buf_size, max_queue_filled - check_size);

    // Enqueue buffer
    uint8_t *buf = (uint8_t *)bm_malloc(buf_size);
    ASSERT_EQ(q_enqueue(queue, buf, buf_size), BmOK);
    bm_free(buf);

    i += buf_size;
  }
  EXPECT_LT(queue->head, queue->tail);

  q_delete(queue);
}

TEST_F(queue_test, dequeue) {
  Q *queue = q_create(queue_buf_size);

  size_t queue_elements = 0;

  // Test failure cases
  size_t fail_queue_size = queue_buf_size / 10;
  uint8_t *fail_buf = (uint8_t *)bm_malloc(fail_queue_size);
  EXPECT_EQ(q_dequeue(NULL, fail_buf, fail_queue_size), BmEINVAL);
  EXPECT_EQ(q_dequeue(queue, NULL, fail_queue_size), BmEINVAL);
  EXPECT_EQ(q_dequeue(queue, fail_buf, 0), BmEINVAL);
  bm_free(fail_buf);

  // Enqueue until full
  uint8_t buf_size = RND.rnd_int(UINT8_MAX, 1);
  uint8_t *buf = (uint8_t *)bm_malloc(buf_size);
  while (q_enqueue(queue, buf, buf_size) != BmENOMEM) {
    queue_elements++;
  }
  bm_free(buf);

  // Test dequeueing wrap around:
  //   - First dequeue the buffer so tail == head
  //   - Then write about 1/2 of the size of the queue to wrap head around 0 index
  //   - Dequeue about 1/4 of the size of the queue
  //   - Assert that tail < head
  uint8_t *dequeue_buf = (uint8_t *)bm_malloc(queue_buf_size);
  for (size_t i = 0; i < queue_elements; i++) {
    ASSERT_EQ(q_dequeue(queue, dequeue_buf, queue_buf_size), BmOK);
  }
  bm_free(dequeue_buf);
  EXPECT_EQ(queue->count, 0);
  queue_elements = 0;

  size_t max_queue_filled = queue_buf_size / 2;
  for (size_t i = 0; i < max_queue_filled;) {
    uint8_t buf_size = RND.rnd_int(UINT8_MAX, 1);

    buf_size =
        i + buf_size < max_queue_filled ? buf_size : max_queue_filled - i;

    // Enqueue buffer
    uint8_t *buf = (uint8_t *)bm_malloc(buf_size);
    ASSERT_EQ(q_enqueue(queue, buf, buf_size), BmOK);
    bm_free(buf);

    i += buf_size;
    queue_elements++;
  }

  dequeue_buf = (uint8_t *)bm_malloc(queue_buf_size);
  for (size_t i = 0; i < queue_elements / 2; i++) {
    ASSERT_EQ(q_dequeue(queue, dequeue_buf, queue_buf_size), BmOK);
  }
  bm_free(dequeue_buf);

  EXPECT_LT(queue->tail, queue->head);

  q_delete(queue);
}

TEST_F(queue_test, size_clear) {
  Q *queue = q_create(queue_buf_size);

  // Test failure cases
  EXPECT_EQ(q_size(NULL), 0);
  EXPECT_EQ(q_clear(NULL), BmEINVAL);

  constexpr size_t queue_elements = queue_buf_size / 32;
  for (size_t i = 0; i < queue_elements; i++) {
    // Enqueue buffer
    uint8_t buf = RND.rnd_int(UINT8_MAX, 0);
    ASSERT_EQ(q_enqueue(queue, &buf, sizeof(buf)), BmOK);
  }

  EXPECT_EQ(q_size(queue), queue_elements);

  // Clear the queue and check the size
  EXPECT_EQ(q_clear(queue), BmOK);
  EXPECT_EQ(q_size(queue), 0);
}
