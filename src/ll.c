#include "ll.h"
#include <stddef.h>

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
static BMErr ll_advance_cursor(LL *ll, void **data) {
  BMErr ret = BmEINVAL;

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
static BMErr ll_reset_cursor(LL *ll) {
  BMErr ret = BmEINVAL;

  if (ll) {
    ll->cursor = NULL;
    ret = BmOK;
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
BMErr ll_item_add(LL *ll, LLItem *node) {
  BMErr ret = BmEINVAL;
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
BMErr ll_get_item(LL *ll, uint32_t id, void **data) {
  BMErr ret = BmEINVAL;
  LLItem_t *current = NULL;
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

 @details Remove item at specified id in linked list

 @param ll list to delete item from
 @param id identifier of item to delete

 @return BmOK on success
 @return BmError on failure
 */
BMErr ll_remove(LL *ll, uint32_t id) {
  BMErr ret = BmEINVAL;
  LLItem **current = NULL;
  if (ll) {
    current = &ll->head;
    ret = BmENODEV;
    while (*current != NULL && (*current)->id != id) {
      current = &(*current)->next;
    }
    if (*current) {
      ret = BmOK;
      if (*current == ll->head) {
        ll->head = (*current)->next;
      } else {
        if (*current == ll->tail) {
          ll->tail = (*current)->previous;
        }
        *current = (*current)->next;
      }
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
BMErr ll_traverse(LL *ll, LLTraverseCb cb, void *arg) {
  BMErr ret = BmEINVAL;
  void *data = NULL;

  if (ll && cb) {
    ll_reset_cursor(ll);
    do {
      ret = ll_advance_cursor(ll, &data);
      if (ret == 0) {
        ret = cb(data, arg);
      }
    } while (ll->cursor && ret == 0);
  }

  return ret;
}
