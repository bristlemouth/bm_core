#include "ll.h"
#include "mock_middleware.h"

DEFINE_FAKE_VALUE_FUNC(BmErr, bm_middleware_rx, uint16_t, void *, uint64_t,
                       uint32_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_middleware_init);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_middleware_net_tx, uint16_t, const void *,
                       uint32_t, uint16_t, void *);

static LL rx;
static LL tx;

static BmErr add_to_ll(LL *ll, void **cb, uint16_t port) {
  LLItem *item = NULL;
  item = ll_create_item(item, cb, sizeof(BmMiddlewareRxCb), port);
  if (!item) {
    return BmENOMEM;
  }

  return ll_item_add(ll, item);
}

BmErr bm_middleware_add_application(uint16_t port, BmIpAddr dest,
                                    BmMiddlewareRxCb rx_cb,
                                    BmMiddlewareTxPrepCb tx_cb,
                                    BmMiddlewareRoutingCb routing_cb) {
  (void)dest;
  (void)routing_cb;

  BmErr err = add_to_ll(&rx, (void **)&rx_cb, port);
  bm_err_check(err, add_to_ll(&tx, (void **)&tx_cb, port));

  return err;
}

void bm_middleware_invoke_rx_cb(uint16_t port, uint64_t node_id, void *buf,
                                uint32_t size) {
  BmMiddlewareRxCb *cb = NULL;

  BmErr err = ll_get_item(&rx, port, (void **)&cb);
  if (err != BmOK) {
    return;
  }

  if (cb && *cb) {
    (*cb)(node_id, buf, size);
  }
}

void bm_middleware_invoke_tx_cb(uint16_t port, uint64_t node_id, void *buf,
                                uint32_t size) {
  BmMiddlewareRxCb *cb = NULL;

  BmErr err = ll_get_item(&tx, port, (void **)&cb);
  if (err != BmOK) {
    return;
  }

  if (cb && *cb) {
    (*cb)(node_id, buf, size);
  }
}
