#include <stdint.h>
#include "util.h"


typedef struct {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint16_t ms;
} RtcTimeAndDate;

BmErr rtcSet(const RtcTimeAndDate *timeAndDate);
BmErr rtcGet(RtcTimeAndDate *timeAndDate);
uint64_t rtcGetMicroSeconds(RtcTimeAndDate *timeAndDate);