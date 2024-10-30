#include "ll.h"
#include "bm_os.h"
#include <stddef.h>
#include <string.h>

/*!
 @brief Advances Item Cursor On Linked List

 @details Advances the cursor on an element of a linked list and
          retreives its data. This starts from the head. ll->cursor should be
          monitored to determine if the iterator is at the end of the list
          (ll->cursor == NULL).

 @param ll list to iterate cursor on node from
 @param data data of node

 @return BmOK on success
 @return BmError on failure
 */
static BmErr ll_advance_cursor(LL *ll, void **data) {
  BmErr ret = BmEINVAL;

  if (ll && data) {
    ret = BmENODEV;
    if (!ll->cursor) {
      ll->cursor = ll->head;
    }
    if (ll->cursor) {
      *data = ll->cursor->data;
      ll->cursor = ll->cursor->next;
      ret = BmOK;
    }
  }

  return ret;
}

/*!
 @brief Reset List Cursor

 @details Resets list cursor to begin iterative operations from the list's
          head. This should be used once the pop API has reached the end
          of the list.
 
 @param ll list to reset cursor

 @return BmOK on success
 @return BmError on failure
 */
static BmErr ll_reset_cursor(LL *ll) {
  BmErr ret = BmEINVAL;

  if (ll) {
    ll->cursor = NULL;
    ret = BmOK;
  }

  return ret;
}

/*!
 @brief Create A Static Linked List Item
 
 @param item pointer to item to create
 @param data data the item will point to
 @param id unique identifier the item represents
 
 @return BmOK on success
 @return BmError on failure
 */
BmErr ll_create_item_static(LLItem *item, void *data, uint32_t id) {
  BmErr ret = BmEINVAL;
  if (item) {
    ret = BmOK;
    item->data = data;
    item->id = id;
    item->dynamic = 0;
  }
  return ret;
}

/*!
 @brief Create A Dynamic Linked List Item

 @param item pointer to item to create
 @param data data the item will point to
 @param size length of data in bytes
 @param id unique identifier the item represents
 
 @return Pointer to newly created LL item on success
 @return NULL on failure
 */
LLItem *ll_create_item(LLItem *item, void *data, uint32_t size, uint32_t id) {
  void *tmp = NULL;
  item = (LLItem *)bm_malloc(sizeof(LLItem));

  if (item) {
    if (data) {
      tmp = bm_malloc(size);
      memcpy(tmp, data, size);
    }
    item->data = tmp;
    item->id = id;
    item->dynamic = 1;
  }

  return item;
}

/*!
 @brief Delete A Dynamic Linked List Item

 @details This is called automatically in LL remove

 @param item pointer to item to delete (free)

 @return BmOK on success
 @return BmError on failure
 */
BmErr ll_delete_item(LLItem *item) {
  BmErr ret = BmEINVAL;

  if (item) {
    ret = BmOK;
    if (item->dynamic) {
      if (item->data) {
        bm_free(item->data);
      }
      bm_free((void *)item);
    }
  }

  return ret;
}

/*!
 @brief Add A Item To A Linked List

 @details Please ensure that the item artifacts are not destroyed during the
          use of the linked list. One way to ensure this is to make your
          variables statically allocated.
 
 @param ll list to add item to
 @param item item to add to linked list

 @return BmOK on success
 @return BmError on failure
 */
BmErr ll_item_add(LL *ll, LLItem *node) {
  BmErr ret = BmEINVAL;
  if (ll && node) {
    ret = BmOK;
    if (ll->head) {
      node->previous = ll->tail;
      ll->tail->next = node;
      ll->tail = node;
      ll->tail->next = NULL;
    } else {
      ll->head = ll->tail = node;
      ll->head->next = ll->head->previous = NULL;
    }
  }
  return ret;
}

/*!
 @brief Get Data Element From Linked List

 @details Obtain data at specified id of linked list

 @param ll list to add item to
 @param id identifier of item to obtain data
 @param data data at specified identifier

 @return BmOK on success
 @return BmError on failure
 */
BmErr ll_get_item(LL *ll, uint32_t id, void **data) {
  BmErr ret = BmEINVAL;
  LLItem *current = NULL;
  if (ll && data) {
    current = ll->head;
    ret = BmENODEV;
    while (current != NULL && current->id != id) {
      current = current->next;
    }
    if (current) {
      *data = current->data;
      ret = BmOK;
    }
  }
  return ret;
}

/*!
 @brief Remove Item From Linked List

 @details Remove item at specified id in linked list,
          if item was dynamically allocated, it will be freed

 @param ll list to delete item from
 @param id identifier of item to delete

 @return BmOK on success
 @return BmError on failure
 */
BmErr ll_remove(LL *ll, uint32_t id) {
  BmErr ret = BmEINVAL;
  LLItem *previous = NULL;
  LLItem *current = NULL;
  if (ll) {
    current = ll->head;
    ret = BmENODEV;

    while (current != NULL && current->id != id) {
      previous = current;
      current = current->next;
    }
    if (current) {
      if (current == ll->head) {
        ll->head = current->next;
      } else {
        ret = BmOK;
        if (current == ll->tail) {
          ll->tail = current->previous;
        }
        previous->next = current->next;
      }
      ret = ll_delete_item(current);
    }
  }
  return ret;
}

/*!
 @brief Traverse Linked List And Perform Callback Operation

 @details Traverses linked list and performs a callback operation on each
          item of the list.

 @param ll list to traverse
 @param cb callback operation to perform
 @param arg argument passed to callback function

 @return BmOK on success
 @return BmError on failure
 */
BmErr ll_traverse(LL *ll, LLTraverseCb cb, void *arg) {
  BmErr ret = BmEINVAL;
  void *data = NULL;

  if (ll && cb) {
    ll_reset_cursor(ll);
    do {
      ret = ll_advance_cursor(ll, &data);
      if (ret == BmOK) {
        ret = cb(data, arg);
      }
    } while (ll->cursor && ret == BmOK);
  }

  return ret;
}
