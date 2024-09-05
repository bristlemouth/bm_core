#ifndef __BM_LL_H__
#define __BM_LL_H__

#include "util.h"
#include <stdint.h>

typedef struct LLItem {
  struct LLItem *next;
  struct LLItem *previous;
  void *data;
  uint32_t id;
} LLItem;

typedef struct {
  LLItem *head;
  LLItem *tail;
  LLItem *cursor;
} LL;

typedef BMErr (*LLTraverseCb)(void *data, void *arg);

BMErr ll_item_add(LL *ll, LLItem *item);
BMErr ll_get_item(LL *ll, uint32_t id, void **data);
BMErr ll_remove(LL *ll, uint32_t id);
BMErr ll_traverse(LL *ll, LLTraverseCb cb, void *arg);

#endif
