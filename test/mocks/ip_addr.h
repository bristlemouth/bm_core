#ifndef LWIP_HDR_IP_ADDR_H
#define LWIP_HDR_IP_ADDR_H

#include <stdint.h>

struct ip6_addr {
  uint32_t addr[4];
};
typedef struct ip6_addr ip6_addr_t;
typedef ip6_addr_t ip_addr_t;

#endif
