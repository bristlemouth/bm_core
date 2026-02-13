#include "fff.h"
#include "pubsub.h"

DECLARE_FAKE_VALUE_FUNC(BmErr, bm_pub, const char *, const void *, uint16_t,
                        uint8_t, uint8_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_pubsub_init);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_pub_wl, const char *, uint16_t, const void *,
                        uint16_t, uint8_t, uint8_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_sub, const char *, const BmPubSubCb);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_sub_wl, const char *, uint16_t,
                        const BmPubSubCb);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_unsub, const char *, const BmPubSubCb);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_unsub_wl, const char *, uint16_t,
                        const BmPubSubCb);
DECLARE_FAKE_VOID_FUNC(bm_handle_msg, uint64_t, void *, uint32_t);
DECLARE_FAKE_VOID_FUNC(bm_print_subs);
DECLARE_FAKE_VALUE_FUNC(char *, bm_get_subs);
