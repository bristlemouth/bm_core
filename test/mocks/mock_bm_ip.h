#include "bm_os.h"
#include "fff.h"
#include "util.h"

typedef BmErr (*BmUdpPortBindCb)(void *, uint64_t, uint32_t);

DECLARE_FAKE_VALUE_FUNC(BmErr, bm_ip_init);
DECLARE_FAKE_VALUE_FUNC(void *, bm_l2_new, uint32_t);
DECLARE_FAKE_VALUE_FUNC(void *, bm_l2_get_payload, void *);
DECLARE_FAKE_VOID_FUNC(bm_l2_tx_prep, void *, uint32_t);
DECLARE_FAKE_VOID_FUNC(bm_l2_free, void *);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_l2_submit, void *, uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_l2_set_netif, bool);
DECLARE_FAKE_VALUE_FUNC(const char *, bm_ip_get_str, uint8_t);
DECLARE_FAKE_VALUE_FUNC(const void *, bm_ip_get, uint8_t);
DECLARE_FAKE_VOID_FUNC(bm_ip_rx_cleanup, void *);
DECLARE_FAKE_VALUE_FUNC(void *, bm_ip_tx_new, const void *, uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_ip_tx_copy, void *, const void *, uint32_t,
                        uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_ip_tx_perform, void *, const void *);
DECLARE_FAKE_VOID_FUNC(bm_ip_tx_cleanup, void *);
DECLARE_FAKE_VALUE_FUNC(void *, bm_udp_bind_port, uint16_t, BmUdpPortBindCb);
DECLARE_FAKE_VALUE_FUNC(void *, bm_udp_new, uint32_t);
DECLARE_FAKE_VALUE_FUNC(void *, bm_udp_get_payload, void *);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_udp_reference_update, void *);
DECLARE_FAKE_VOID_FUNC(bm_udp_cleanup, void *);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_udp_tx_perform, void *, void *, uint32_t,
                        const void *, uint16_t);
