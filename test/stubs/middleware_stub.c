#include "mock_middleware.h"

DEFINE_FAKE_VALUE_FUNC(BmErr, bm_middleware_rx, uint16_t, void *, uint64_t,
                       uint32_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_middleware_init);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_middleware_net_tx, uint16_t, void *, uint32_t);

static BmMiddlewareRxCb cb = NULL;

BmErr bm_middleware_add_application(uint16_t port, BmIpAddr dest,
                                    BmMiddlewareRxCb rx_cb) {
  (void)port;
  (void)dest;

  if (!rx_cb) {
    return BmEINVAL;
  }

  cb = rx_cb;

  return BmOK;
}

void bm_middleware_invoke_cb(uint16_t port, uint64_t node_id, void *buf,
                             uint32_t size) {
  (void)port;

  if (cb) {
    printf("Here?\n");
    cb(node_id, buf, size);
  }
}
