#include "fff.h"
#include "util.h"
#include <stdint.h>

typedef struct {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint16_t ms;
} RtcTimeAndDate;

DECLARE_FAKE_VALUE_FUNC(BmErr, bm_rtc_set, const RtcTimeAndDate *);
DECLARE_FAKE_VALUE_FUNC(BmErr, bm_rtc_get, RtcTimeAndDate *);
DECLARE_FAKE_VALUE_FUNC(uint64_t, bm_rtc_get_micro_seconds, RtcTimeAndDate *);
