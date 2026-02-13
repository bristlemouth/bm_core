#include "ll.h"
#include "mock_middleware.h"

DEFINE_FAKE_VALUE_FUNC(BmErr, bm_middleware_rx, uint16_t, void *, uint64_t,
                       uint32_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_middleware_init);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_middleware_net_tx, uint16_t, void *, uint32_t);

static LL applications;

BmErr bm_middleware_add_application(uint16_t port, BmIpAddr dest,
                                    BmMiddlewareRxCb rx_cb) {
  (void)dest;

  LLItem *item = NULL;
  item = ll_create_item(item, &rx_cb, sizeof(BmMiddlewareRxCb), port);
  if (!item) {
    return BmENOMEM;
  }

  return ll_item_add(&applications, item);
}

void bm_middleware_invoke_cb(uint16_t port, uint64_t node_id, void *buf,
                             uint32_t size) {
  BmMiddlewareRxCb *cb = NULL;

  BmErr err = ll_get_item(&applications, port, (void **)&cb);
  if (err != BmOK) {
    return;
  }

  if (cb && *cb) {
    (*cb)(node_id, buf, size);
  }
}
