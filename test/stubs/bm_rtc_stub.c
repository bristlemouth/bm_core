#include "mock_bm_rtc.h"

DEFINE_FAKE_VALUE_FUNC(BmErr, bm_rtc_set, const RtcTimeAndDate *);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_rtc_get, RtcTimeAndDate *);
DEFINE_FAKE_VALUE_FUNC(uint64_t, bm_rtc_get_micro_seconds, RtcTimeAndDate *);