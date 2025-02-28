#ifndef __CB_QUEUE_H__
#define __CB_QUEUE_H__

#include "ll.h"
#include "util.h"

typedef BmErr (*BmQueueCb)(const void *arg);

typedef struct {
  LL cb_list;
  uint32_t add_id;
  uint32_t get_id;
} BmCbQueue;

BmErr queue_cb_enqueue(BmCbQueue *q, BmQueueCb cb);
BmErr queue_cb_dequeue(BmCbQueue *q, void *arg, bool invoke);

#endif
