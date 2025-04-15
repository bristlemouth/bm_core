#include "cb_queue.h"

/*!
  @brief Enqueue A Callback Onto The Queue

  @param q queue instance
  @param cb callback to enqueue

  @return BmOK on success, BmErr on failure
 */
BmErr queue_cb_enqueue(BmCbQueue *q, BmQueueCb cb) {
  BmErr err = BmEINVAL;
  LLItem *cb_item = NULL;

  if (q) {
    err = BmENOMEM;
    cb_item = ll_create_item(&cb, sizeof(&cb), q->add_id);

    if (cb_item) {
      err = ll_item_add(&q->cb_list, cb_item);
      if (err == BmOK) {
        q->add_id++;
      }
    }
  }

  return err;
}

/*!
  @brief Dequeue A Callback From The Queue

  @details The callback from the queue does not need to be invoked, it can be
           bypassed if necessary

  @param q queue instance
  @param arg argument to pass to the callback invoked
  @param invoke true if the callback is to be invoked, false otherwise

  @return BmOK on success, BmErr on failure
 */
BmErr queue_cb_dequeue(BmCbQueue *q, void *arg, bool invoke) {
  BmErr err = BmEINVAL;
  BmQueueCb *cb = NULL;

  if (!q) {
    return err;
  }

  err = BmECANCELED;
  if (q->add_id != q->get_id) {
    err = ll_get_item(&q->cb_list, q->get_id, (void **)&cb);

    if (err == BmOK) {
      if (*cb && invoke) {
        err = (*cb)(arg);
      }
      ll_remove(&q->cb_list, q->get_id);
    }

    q->get_id++;
  }

  return err;
}
