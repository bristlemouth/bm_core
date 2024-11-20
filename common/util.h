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
/* We store the egress port for the RX device and the ingress port of the TX device
   in bytes 5 (index 4) and 6 (index 5) of the SRC IPv6 address. */
#define egress_port_idx 4
#define ingress_port_idx 5

// Ethernet Header Sizes
#define ethernet_destination_size_bytes 6
#define ethernet_src_size_bytes 6
#define ethernet_type_size_bytes 2

// IPV6 Header Sizes
#define ipv6_version_traffic_class_flow_label_size_bytes 4
#define ipv6_payload_length_size_bytes 2
#define ipv6_next_header_size_bytes 1
#define ipv6_hop_limit_size_bytes 1
#define ipv6_source_address_size_bytes 16
#define ipv6_destination_address_size_bytes 16

// UDP Header Sizes
#define udp_src_size_bytes 2
#define udp_destination_size_bytes 2
#define udp_length_size_bytes 2
#define udp_checksum_size_bytes 2

// Ethernet Offsets
#define ethernet_destination_offset (0)
#define ethernet_src_offset                                                    \
  (ethernet_destination_offset + ethernet_destination_size_bytes)
#define ethernet_type_offset (ethernet_src_offset + ethernet_src_size_bytes)

// Ethernet Getters
#define ethernet_get_type(buf) (uint8_to_uint16(&buf[ethernet_type_offset]))

// IPV6 Offsets
#define ipv6_version_traffic_class_flow_label_offset                           \
  (ethernet_type_offset + ethernet_type_size_bytes)
#define ipv6_payload_length_offset                                             \
  (ipv6_version_traffic_class_flow_label_offset +                              \
   ipv6_version_traffic_class_flow_label_size_bytes)
#define ipv6_next_header_offset                                                \
  (ipv6_payload_length_offset + ipv6_payload_length_size_bytes)
#define ipv6_hop_limit_offset                                                  \
  (ipv6_next_header_offset + ipv6_next_header_size_bytes)
#define ipv6_source_address_offset                                             \
  (ipv6_hop_limit_offset + ipv6_hop_limit_size_bytes)
#define ipv6_destination_address_offset                                        \
  (ipv6_source_address_offset + ipv6_source_address_size_bytes)

// IPV6 Getters
#define ipv6_get_version_traffic_class_flow_label(buf)                         \
  (uint8_to_uint32(&buf[ipv6_version_traffic_class_flow_label_offset]))
#define ipv6_get_payload_length(buf)                                           \
  (uint8_to_uint16(&buf[ipv6_payload_length_offset]))
#define ipv6_get_next_header(buf) (buf[ipv6_next_header_offset])
#define ipv6_get_hop_limit(buf) (buf[ipv6_hop_limit_offset])

// UDP Offsets
#define udp_src_offset                                                         \
  (ipv6_destination_address_offset + ipv6_destination_address_size_bytes)
#define udp_destination_offset (udp_src_offset + udp_src_size_bytes)
#define udp_length_offset (udp_destination_offset + udp_destination_size_bytes)
#define udp_checksum_offset (udp_length_offset + udp_length_size_bytes)

// Network Helper Macros
#define add_egress_port(addr, port)                                            \
  (addr[ipv6_source_address_offset + egress_port_idx] = port)
#define add_ingress_port(addr, port)                                           \
  (addr[ipv6_source_address_offset + ingress_port_idx] = port)

void network_add_egress_port(uint8_t *payload, uint8_t port);

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

uint32_t time_remaining(uint32_t start, uint32_t current, uint32_t timeout);
uint32_t utc_from_date_time(uint16_t year, uint8_t month, uint8_t day,
                            uint8_t hour, uint8_t minute, uint8_t second);
void date_time_from_utc(uint64_t utc_us, UtcDateTime *date_time);

#define bm_err_check(e, f)                                                     \
  if (e == BmOK) {                                                             \
    e = f;                                                                     \
    if (e != BmOK) {                                                           \
      printf("err: %d at %s:%d\n", e, __FILE__, __LINE__);                     \
    }                                                                          \
  }
#define bm_err_check_print(e, f, format, ...)                                  \
  if (e == BmOK) {                                                             \
    e = f;                                                                     \
    if (e != BmOK) {                                                           \
      printf("err: %d at %s:%d\n" format, e, __FILE__, __LINE__, __VA_ARGS__); \
    }                                                                          \
  }

//TODO: this is a placeholder until ip addresses are figured out
typedef struct {
  uint32_t addr[4];
  uint8_t reserved;
} bm_ip_addr;

extern const bm_ip_addr multicast_global_addr;
extern const bm_ip_addr multicast_ll_addr;

bool is_little_endian(void);
void swap_16bit(void *x);
void swap_32bit(void *x);
void swap_64bit(void *x);
size_t bm_strnlen(const char *s, size_t max_length);

static inline uint16_t uint8_to_uint16(uint8_t *buf) {
  return (uint16_t)(buf[0] | buf[1] << 8);
}

static inline uint32_t uint8_to_uint32(uint8_t *buf) {
  return (uint32_t)(buf[0] | buf[1] << 8 | buf[1] << 16 | buf[1] << 24);
}

//TODO: make this endian agnostic and platform agnostic
/*!
  Extract 64-bit node id from IP address

  \param *ip address to extract node id from
  \return node id
*/
static inline uint64_t ip_to_nodeid(void *ip) {
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
