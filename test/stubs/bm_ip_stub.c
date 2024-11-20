#include "mock_bm_ip.h"
#include "util.h"

DEFINE_FAKE_VALUE_FUNC(BmErr, bm_ip_init);
DEFINE_FAKE_VALUE_FUNC(void *, bm_l2_new, uint32_t);
DEFINE_FAKE_VALUE_FUNC(void *, bm_l2_get_payload, void *);
DEFINE_FAKE_VOID_FUNC(bm_l2_tx_prep, void *, uint32_t);
DEFINE_FAKE_VOID_FUNC(bm_l2_free, void *);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_l2_submit, void *, uint32_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_l2_set_netif, bool);
DEFINE_FAKE_VALUE_FUNC(const char *, bm_ip_get_str, uint8_t);
DEFINE_FAKE_VALUE_FUNC(const void *, bm_ip_get, uint8_t);
DEFINE_FAKE_VOID_FUNC(bm_ip_rx_cleanup, void *);
DEFINE_FAKE_VALUE_FUNC(void *, bm_ip_tx_new, const void *, uint32_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_ip_tx_copy, void *, const void *, uint32_t,
                       uint32_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_ip_tx_perform, void *, const void *);
DEFINE_FAKE_VOID_FUNC(bm_ip_tx_cleanup, void *);
DEFINE_FAKE_VALUE_FUNC(void *, bm_udp_bind_port, uint16_t, BmUdpPortBindCb);
DEFINE_FAKE_VALUE_FUNC(void *, bm_udp_new, uint32_t);
DEFINE_FAKE_VALUE_FUNC(void *, bm_udp_get_payload, void *);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_udp_reference_update, void *);
DEFINE_FAKE_VOID_FUNC(bm_udp_cleanup, void *);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_udp_tx_perform, void *, void *, uint32_t,
                       const void *, uint16_t);
