#include "q.h"
#include "bm_os.h"
#include <string.h>

typedef struct {
  uint32_t size;
  uint8_t buf[];
} QItem;

/*!
 @brief Get Amount Of Free Space In Bytes In The Queue Buffer

 @details Because this queue utilizes a circular buffer, this API obtains the
          free space that has not been consumed by the head index.

 @param queue queue to examine

 @return Amount of free space in bytes in the queue buffer
 */
static uint32_t q_get_free_space(Q *queue) {
  if (queue->head >= queue->tail) {
    return (queue->size - queue->head) + queue->tail;
  } else {
    return queue->tail - queue->head;
  }
}

/*!
 @brief Increment An Index Of The Queue

 @details This is intended to move the head or tail indices within the
          queue by one element and wrap around once they have reached
          the end of the queue's buffer size.

 @param queue queue to increment index
 @param idx pointer to head or tail element of the queue
 */
static inline void q_increment(Q *queue, uint32_t *idx) {
  *idx = (*idx + 1) % queue->size;
}

/*!
 @brief Create A Dynamically Allocated Queue

 @details Creates the queue struct and a buffer of size parameter.

 @param size Size of buffer to store items on the queue

 @return Pointer to created queue on success
         NULL on failure
 */
Q *q_create(uint32_t size) {
  if (!size) {
    return NULL;
  }

  Q *queue = (Q *)bm_malloc(sizeof(Q));

  if (!queue) {
    return NULL;
  }

  memset(queue, 0, sizeof(Q));
  queue->size = size;
  queue->buf = bm_malloc(size);
  if (!queue->buf) {
    bm_free(queue);
    queue = NULL;
  }

  return queue;
}

/*!
 @brief Free A Dynamically Allocated Queue

 @param queue queue element to free

 @return BmOK on success
         BmEINVAL if queue is NULL
 */
BmErr q_delete(Q *queue) {
  if (!queue) {
    return BmEINVAL;
  }

  bm_free(queue->buf);
  queue->buf = NULL;
  bm_free(queue);
  queue = NULL;

  return BmOK;
}

/*!
 @brief Create A Statically Allocated Queue

 @details Initializes a queue with static elements. Both queue and buf must be
          statically allocated to preserve the state of the queue during
          runtime.

 @param queue statically allocated queue to create
 @param buf statically allocated buffer to hold queue elements
 @param size size of buf

 @return BmOK on success
         BmEINVAL if invalid parameters
 */
BmErr q_create_static(Q *queue, uint8_t *buf, uint32_t size) {
  if (!queue || !buf || !size) {
    return BmEINVAL;
  }

  memset(queue, 0, sizeof(Q));
  queue->buf = buf;
  queue->size = size;

  return BmOK;
}

/*!
 @brief Enqueue Data To The Queue

 @details A QItem is created to add metadata about the data being enqueued,
          this is copied to the queue's buffer. Data is then copied to the
          queue's buffer as well.
           

 @param queue queue to enqueue data to
 @param data data to enqueue
 @param size size of data

 @return BmOK on success
         BmEINVAL if invalid parameters
         BmENOMEM if not enough space in the queue's buffer to enqueue data
 */
BmErr q_enqueue(Q *queue, const void *data, uint32_t size) {
  if (!queue || !data || !size) {
    return BmEINVAL;
  }

  if (q_get_free_space(queue) < (size + sizeof(QItem))) {
    return BmENOMEM;
  }

  QItem item = {size};
  QItem *p_item = &item;

  // Copy queue item to buffer
  for (uint8_t i = 0; i < sizeof(QItem); i++) {
    queue->buf[queue->head] = ((uint8_t *)p_item)[i];
    q_increment(queue, &queue->head);
  }

  // Copy data to queue
  for (uint32_t i = 0; i < size; i++) {
    queue->buf[queue->head] = ((uint8_t *)data)[i];
    q_increment(queue, &queue->head);
  }

  queue->count++;

  return BmOK;
}

/*!
 @brief Dequeue Data From The Queue

 @details Data is dequeued onto the data parameters. The size parameter must be
          large enough to hold the queue element attempting to be dequeued.

 @param queue queue to dequeue data from
 @param data pointer to data buffer to hold dequeued element
 @param size size of data buffer

 @return BmOK on success
         BmEINVAL if invalid parameters
         BmENODATA if there are no elements in the queue to dequeue
         BmENOMEM if size of data is smaller than the size of the element being
                  dequeued
 */
BmErr q_dequeue(Q *queue, void *data, uint32_t size) {
  if (!queue || !data || !size) {
    return BmEINVAL;
  }

  if (!queue->count) {
    return BmENODATA;
  }

  QItem *p_item = (QItem *)&queue->buf[queue->tail];

  if (p_item->size > size) {
    return BmENOMEM;
  }

  // Consume the QItem struct
  queue->tail += sizeof(QItem);

  // Copy queue item to buffer
  uint8_t *p_data = (uint8_t *)data;
  for (uint32_t i = 0; i < p_item->size; i++) {
    p_data[i] = queue->buf[queue->tail];
    q_increment(queue, &queue->tail);
  }

  queue->count--;

  return BmOK;
}

/*!
 @brief Obtain The Number Of Elements In The Queue

 @param queue queue to obtain size from

 @return Number of items in the queue or 0 if queue is NULL
 */
uint32_t q_size(Q *queue) {
  if (!queue) {
    return 0;
  }

  return queue->count;
}

/*!
 @brief Clear Elements In The Queue

 @param queue

 @return BmOK on success
         BmEINVAL if invalid parameters
 */
BmErr q_clear(Q *queue) {
  if (!queue) {
    return BmEINVAL;
  }

  queue->count = 0;
  queue->head = 0;
  queue->tail = 0;

  return BmOK;
}
