#include "util.h"
#include <stdint.h>

typedef struct LLItem {
  struct LLItem *next;
  struct LLItem *previous;
  void *data;
  uint32_t id;
  uint8_t dynamic;
} LLItem;

typedef struct {
  LLItem *head;
  LLItem *tail;
  LLItem *cursor;
} LL;

typedef BmErr (*LLTraverseCb)(void *data, void *arg);

BmErr ll_create_item_static(LLItem *item, void *data, uint32_t id);
LLItem *ll_create_item(LLItem *item, void *data, uint32_t id);
BmErr ll_delete_item(LLItem *item);
BmErr ll_item_add(LL *ll, LLItem *item);
BmErr ll_get_item(LL *ll, uint32_t id, void **data);
BmErr ll_remove(LL *ll, uint32_t id);
BmErr ll_traverse(LL *ll, LLTraverseCb cb, void *arg);
