#include "queue.h"
#include "bm_os.h"
#include <string.h>

typedef struct {
  uint32_t size;
  uint8_t buf[];
} QItem;

static uint32_t q_get_free_space(Q *queue) {
  if (queue->head >= queue->tail) {
    return (queue->size - queue->head) + queue->tail;
  } else {
    return queue->tail - queue->head;
  }
}

static inline void q_increment(Q *queue, uint32_t *idx) {
  *idx = (*idx + 1) % queue->size;
}

Q *q_create(uint32_t size) {
  if (!size) {
    return NULL;
  }

  Q *queue = (Q *)bm_malloc(sizeof(Q));

  memset(queue, 0, sizeof(Q));
  if (!queue) {
    return NULL;
  }

  queue->size = size;
  queue->buf = bm_malloc(size);
  if (!queue->buf) {
    bm_free(queue);
    queue = NULL;
  }

  return queue;
}

BmErr q_free(Q *queue) {
  if (!queue) {
    return BmEINVAL;
  }

  bm_free(queue->buf);
  bm_free(queue);

  return BmOK;
}

BmErr q_create_static(Q *queue, uint8_t *buf, uint32_t size) {
  if (!queue || !buf || !size) {
    return BmEINVAL;
  }

  memset(queue, 0, sizeof(Q));
  queue->buf = buf;
  queue->size = size;

  return BmOK;
}

BmErr q_enqueue(Q *queue, void *data, uint32_t size) {
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

  // Copy buffer to queue
  for (uint32_t i = 0; i < size; i++) {
    queue->buf[queue->head] = ((uint8_t *)data)[i];
    q_increment(queue, &queue->head);
  }

  queue->count++;

  return BmOK;
}

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
