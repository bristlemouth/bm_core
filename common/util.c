#include "util.h"
#include <stdint.h>

const BmIpAddr multicast_global_addr = {{
    0xFF,
    0x03,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x01,
}};
const BmIpAddr multicast_ll_addr = {{
    0xFF,
    0x02,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x01,
}};

uint32_t time_remaining(uint32_t start, uint32_t current, uint32_t timeout) {
  int32_t remaining = (int32_t)((start + timeout) - current);

  if (remaining < 0) {
    remaining = 0;
  }

  return remaining;
}

bool is_global_multicast(const BmIpAddr *dst_ip) {
  bool is_global_multi = false;
  if (dst_ip != NULL) {
    // Global multicast address is FF03::1
    is_global_multi = dst_ip->addr[0] == multicast_global_addr.addr[0] && // FF
                      dst_ip->addr[1] == multicast_global_addr.addr[1] && // 03
                      dst_ip->addr[15] == multicast_global_addr.addr[15]; // 01
  }
  return is_global_multi;
}

bool is_link_local_multicast(const BmIpAddr *dst_ip) {
  bool is_ll_multi = false;
  if (dst_ip != NULL) {
    // Link-local multicast address is FF02::1
    is_ll_multi = dst_ip->addr[0] == multicast_ll_addr.addr[0] && // FF
                  dst_ip->addr[1] == multicast_ll_addr.addr[1] && // 02
                  dst_ip->addr[15] == multicast_ll_addr.addr[15]; // 01
  }
  return is_ll_multi;
}

bool is_little_endian(void) {
  static const uint32_t endianness = 1;
  return (*(uint8_t *)&endianness);
}

void swap_16bit(void *x) {
  uint16_t *swp = (uint16_t *)x;
  *swp = (*swp << 8) | (*swp >> 8);
}
void swap_32bit(void *x) {
  uint32_t *swp = (uint32_t *)x;
  *swp = (*swp << 24) | ((*swp & 0xFF00) << 8) | ((*swp >> 8) & 0xFF00) |
         (*swp >> 24);
}
void swap_64bit(void *x) {
  uint64_t *swp = (uint64_t *)x;
  *swp = (*swp << 56) | ((*swp & 0x0000FF00) << 40) |
         ((*swp & 0x00FF0000) << 24) | ((*swp & 0xFF000000) << 8) |
         ((*swp >> 8) & 0xFF000000) | ((*swp >> 24) & 0x00FF0000) |
         ((*swp >> 40) & 0x0000FF00) | (*swp >> 56);
}

size_t bm_strnlen(const char *s, size_t max_length) {
  const char *start = s;
  if (s) {
    while (max_length-- > 0 && *s) {
      s++;
    }
  }
  return s - start;
}

/*!
 @brief Match A Wildcard Pattern To A String

 @details Matches a wildcard pattern (currently only supports '*' and '?') to a
          string, can also be used as a memcmp function.

 @param str string to match to pattern
 @param str_len length of string
 @param pattern pattern string to compare against str
 @param pattern_len length of pattern string

 @return true if match, false if it does not match
 */
bool bm_wildcard_match(const char *str, uint16_t str_len, const char *pattern,
                       uint16_t pattern_len) {
  int32_t star_idx = -1;
  uint16_t match_idx = 0;
  uint16_t i = 0, j = 0;

  while (i < str_len) {
    if (j < pattern_len && (pattern[j] == '?' || str[i] == pattern[j])) {
      i++;
      j++;
    } else if (j < pattern_len && pattern[j] == '*') {
      star_idx = j++;
      match_idx = i;
    } else if (star_idx != -1) {
      // No match but a previous wildcard is found, backtrack to last '*'
      j = star_idx + 1;
      i = ++match_idx;
    } else {
      break;
    }
  }

  // Skip trailing '*' in pattern
  while (j < pattern_len && pattern[j] == '*') {
    j++;
  }

  return j == pattern_len;
}

// leap year calulator expects year argument as years offset from 1970
#define leap_year(Y)                                                           \
  (((1970 + Y) > 0) && !((1970 + Y) % 4) &&                                    \
   (((1970 + Y) % 100) || !((1970 + Y) % 400)))

#define secs_per_min (60UL)
#define secs_per_hour (3600UL)
#define secs_per_day (secs_per_hour * 24UL)
#define microseconds_per_second (1000000)

static const uint8_t MONTH_DAYS[] = {
    31, 28, 31, 30, 31, 30, 31,
    31, 30, 31, 30, 31}; // API starts months from 1, this array starts from 0
static const uint8_t MONTH_DAYS_LEAP_YEAR[] = {
    31, 29, 31, 30, 31, 30, 31,
    31, 30, 31, 30, 31}; // API starts months from 1, this array starts from 0

/*!
  Convert date (YYYYMMDDhhmmss) into unix timestamp(UTC)

  \param[in] year - Full year (2022, for example)
  \param[in] month - Month starting at 1
  \param[in] day - Day starting at 1
  \param[in] hour - Hour starting at 0 and ending at 23
  \param[in] minute - Minute starting at 0 and ending at 59
  \param[in] second - Minute starting at 0 and ending at 59
  \return UTC seconds since Jan 1st 1970 00:00:00
*/
uint32_t utc_from_date_time(uint16_t year, uint8_t month, uint8_t day,
                            uint8_t hour, uint8_t minute, uint8_t second) {
  int i;
  uint32_t seconds;

  // seconds from 1970 till 1 jan 00:00:00 of the given year
  seconds = (year - 1970) * (secs_per_day * 365);
  for (i = 1970; i < year; i++) {
    if (leap_year(i - 1970)) {
      seconds += secs_per_day; // add extra days for leap years
    }
  }

  // add days for this year, months start from 1
  for (i = 1; i < month; i++) {
    if ((i == 2) && leap_year(year - 1970)) {
      seconds += secs_per_day * 29;
    } else {
      seconds +=
          secs_per_day * MONTH_DAYS[i - 1]; //monthDay array starts from 0
    }
  }
  seconds += (day - 1) * secs_per_day;
  seconds += hour * secs_per_hour;
  seconds += minute * secs_per_min;
  seconds += second;
  return seconds;
}

#define days_per_year (365)
#define days_per_leap_year (days_per_year + 1)

static uint32_t get_days_for_year(uint32_t year) {
  return leap_year(year - 1970) ? days_per_leap_year : days_per_year;
}

static const uint8_t *get_days_per_month(uint32_t year) {
  return leap_year(year - 1970) ? MONTH_DAYS_LEAP_YEAR : MONTH_DAYS;
}

/*!
  Convert unix timestamp(UTC) in microseconds to date (YYYYMMDDhhmmss)

  \param[in] utc_us - utc in microseconds
  \param[out] UtcDateTime - date time
  \return None
*/
void date_time_from_utc(uint64_t utc_us, UtcDateTime *date_time) {
  if (date_time) {
    // year
    uint64_t days = (utc_us / microseconds_per_second) / secs_per_day;
    date_time->year = 1970;
    while (days >= get_days_for_year(date_time->year)) {
      days -= get_days_for_year(date_time->year);
      date_time->year++;
    }

    // months
    const uint8_t *monthDays = get_days_per_month(date_time->year);
    date_time->month = 1;
    while (days >= monthDays[date_time->month - 1]) {
      days -= monthDays[date_time->month - 1];
      date_time->month++;
    }

    // days
    date_time->day = days + 1;

    uint64_t secondsRemaining =
        (utc_us / microseconds_per_second) % secs_per_day;
    // hours
    date_time->hour = secondsRemaining / secs_per_hour;

    // minutes
    date_time->min = (secondsRemaining / secs_per_min) % secs_per_min;

    // seconds
    date_time->sec = secondsRemaining % secs_per_min;

    // useconds
    date_time->usec = utc_us % microseconds_per_second;
  }
}
