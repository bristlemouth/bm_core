#include "cb_queue.h"
#include "fff.h"

DECLARE_FAKE_VALUE_FUNC(BmErr, queue_cb_enqueue, BmCbQueue *, BmQueueCb);
DECLARE_FAKE_VALUE_FUNC(BmErr, queue_cb_dequeue, BmCbQueue *, void *, bool);
