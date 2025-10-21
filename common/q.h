#include "util.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Q {
  uint32_t size;
  uint32_t head;
  uint32_t tail;
  uint32_t count;
  uint8_t *buf;
} Q;

Q *q_create(uint32_t size);
BmErr q_delete(Q *queue);
BmErr q_create_static(Q *queue, uint8_t *buf, uint32_t size);
BmErr q_enqueue(Q *queue, const void *data, uint32_t size);
BmErr q_dequeue(Q *queue, void *data, uint32_t size);

#ifdef __cplusplus
}
#endif
