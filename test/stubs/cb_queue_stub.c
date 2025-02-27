#include "mock_cb_queue.h"

DEFINE_FAKE_VALUE_FUNC(BmErr, queue_cb_enqueue, BmCbQueue *, BmQueueCb);
DEFINE_FAKE_VALUE_FUNC(BmErr, queue_cb_dequeue, BmCbQueue *, void *, bool);
