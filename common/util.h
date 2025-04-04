#ifndef __BM_UTIL_H__
#define __BM_UTIL_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// Error Return Values
typedef enum {
  BmOK = 0,
  BmEPERM = 1,
  BmENOENT = 2,
  BmEIO = 5,
  BmENXIO = 6,
  BmEAGAIN = 11,
  BmENOMEM = 12,
  BmEACCES = 13,
  BmENODEV = 19,
  BmEINVAL = 22,
  BmENODATA = 61,
  BmENONET = 64,
  BmEBADMSG = 74,
  BmEMSGSIZE = 90,
  BmENETDOWN = 100,
  BmETIMEDOUT = 110,
  BmECONNREFUSED = 111,
  BmEHOSTDOWN = 112,
  BmEHOSTUNREACH = 113,
  BmEALREADY = 114,
  BmEINPROGRESS = 115,
  BmECANCELED = 125,
  BmENOTINTREC = 140,
} BmErr;

// Task Priorities
#ifndef bcmp_task_priority
#define bcmp_task_priority 5
#endif

#ifndef bcmp_topo_task_priority
#define bcmp_topo_task_priority 3
#endif

// IP Stack Related Items
#define ethernet_type_ipv6 (0x86DDU)
#define ip_proto_udp (17)
#define ip_proto_bcmp (0xBC)

typedef struct {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t min;
  uint8_t sec;
  uint32_t usec;
} UtcDateTime;

#define array_size(x) (sizeof(x) / sizeof(x[0]))
#define member_size(type, member) (sizeof(((type *)0)->member))
#define s_to_ms(x) (x * 1000)
#define ms_to_s(x) (x / 1000)

uint32_t time_remaining(uint32_t start, uint32_t current, uint32_t timeout);
uint32_t utc_from_date_time(uint16_t year, uint8_t month, uint8_t day,
                            uint8_t hour, uint8_t minute, uint8_t second);
void date_time_from_utc(uint64_t utc_us, UtcDateTime *date_time);

#define bm_err_report(e, f)                                                    \
  e = f;                                                                       \
  if (e != BmOK) {                                                             \
    bm_debug("err: %d at %s:%d\n", e, __FILE__, __LINE__);                     \
  }
#define bm_err_report_print(e, f, format, ...)                                 \
  e = f;                                                                       \
  if (e != BmOK) {                                                             \
    bm_debug("err: %d at %s:%d " format "\n", e, __FILE__, __LINE__,           \
             __VA_ARGS__);                                                     \
  }
#define bm_err_check(e, f)                                                     \
  if (e == BmOK) {                                                             \
    e = f;                                                                     \
    if (e != BmOK) {                                                           \
      bm_debug("err: %d at %s:%d\n", e, __FILE__, __LINE__);                   \
    }                                                                          \
  }
#define bm_err_check_print(e, f, format, ...)                                  \
  if (e == BmOK) {                                                             \
    e = f;                                                                     \
    if (e != BmOK) {                                                           \
      bm_debug("err: %d at %s:%d " format "\n", e, __FILE__, __LINE__,         \
               __VA_ARGS__);                                                   \
    }                                                                          \
  }

typedef struct {
  uint8_t addr[16];
} BmIpAddr;

extern const BmIpAddr multicast_global_addr;
extern const BmIpAddr multicast_ll_addr;

bool is_global_multicast(const BmIpAddr *dst_ip);
bool is_link_local_multicast(const BmIpAddr *dst_ip);
bool is_little_endian(void);
void swap_16bit(void *x);
void swap_32bit(void *x);
void swap_64bit(void *x);
size_t bm_strnlen(const char *s, size_t max_length);

static inline uint16_t uint8_to_uint16(uint8_t *buf) {
  return (uint16_t)(buf[1] | buf[0] << 8);
}

static inline uint32_t uint8_to_uint32(uint8_t *buf) {
  return (uint32_t)(buf[3] | buf[2] << 8 | buf[1] << 16 | buf[0] << 24);
}

//TODO: make this endian agnostic and platform agnostic
/*!
  Extract 64-bit node id from IP address

  \param *ip address to extract node id from
  \return node id
*/
static inline uint64_t ip_to_nodeid(const BmIpAddr *ip) {
  uint32_t high_word = 0;
  uint32_t low_word = 0;
  if (ip && is_little_endian()) {
    high_word = ((uint32_t *)(ip))[2];
    low_word = ((uint32_t *)(ip))[3];
    swap_32bit(&high_word);
    swap_32bit(&low_word);
  }
  return (uint64_t)high_word << 32 | (uint64_t)low_word;
}

#endif
