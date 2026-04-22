/// @file bm_linux.c
/// @brief Linux hosted IP stack implementation of bm_ip.h APIs for bm_sbc.
///
/// Replaces lwIP with a pure-software IP stack that manually constructs and
/// parses Ethernet + IPv6 frames in malloc'd buffers.  Actual wire I/O is
/// delegated to the L2 layer (l2.c) via bm_l2_link_output().

#include "bcmp.h"
#include "bm_config.h"
#include "bm_ip.h"
#include "bm_os.h"
#include "device.h"
#include "l2.h"
#include "ll.h"
#include "packet.h"
#include "util.h"

#include <arpa/inet.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Data structures
// ---------------------------------------------------------------------------

/// Malloc-based buffer with reference counting.  Replaces lwIP's struct pbuf.
/// A single allocation holds the header and payload contiguously thanks to the
/// flexible array member.
typedef struct {
  uint32_t ref;        ///< Reference count (starts at 1 on allocation).
  uint32_t alloc_size; ///< Total allocated payload capacity in bytes.
  uint32_t len;        ///< Current valid data length in bytes (<= alloc_size).
  uint8_t payload[];   ///< Frame data (flexible array member).
} LinuxBuf;

/// Abstraction passed through the BCMP packet processor (packet.c).
/// Mirrors LwipLayout: the accessor callbacks registered via packet_init()
/// extract data/src/dst from this struct.
///
/// Ownership semantics differ between TX and RX:
///   TX: data is a separate malloc'd buffer; src points to static CTX address;
///       dst is the caller's pointer (not copied).  bm_ip_tx_cleanup frees
///       data and the layout itself.
///   RX: data, src, and dst are all independently malloc'd copies.
///       bm_ip_rx_cleanup frees all three plus the layout.
typedef struct {
  void *data;          ///< Pointer to upper-layer payload (BCMP message bytes).
  uint32_t data_size;  ///< Size of the data buffer in bytes.
  const BmIpAddr *src; ///< Source IPv6 address.
  const BmIpAddr *dst; ///< Destination IPv6 address.
} LinuxLayout;

/// Opaque handle returned by bm_udp_bind_port().  Stores the bound local port
/// so that bm_udp_tx_perform() can fill the UDP source-port header field.
typedef struct {
  uint16_t port; ///< Bound local UDP port number.
} LinuxUdpPcb;

/// Callback holder stored in the UDP linked list (CTX.udp_list).
/// Keyed by port number (uint32_t) — avoids the 64-bit pointer truncation
/// issue present in the lwIP implementation's (uint32_t)pcb cast.
typedef struct {
  BmErr (*udp_cb)(uint16_t, void *, uint64_t, uint32_t);
} UdpCb;

// ---------------------------------------------------------------------------
// Static context — global state for the Linux IP stack
// ---------------------------------------------------------------------------

static struct {
  BmIpAddr ll_addr;        ///< Link-local address:  fe80::<node_id>
  BmIpAddr unicast_addr;   ///< Unique-local address: fd00::<node_id>
  BmIpAddr multicast_addr; ///< Global multicast:     ff03::1
  LL udp_list;             ///< Linked list of UdpCb entries, keyed by port.
  char ip_str[2][40];      ///< Pre-formatted address strings (0=ll, 1=unicast).
  bool link_up;            ///< Network interface up/down state.
} CTX;

// ---------------------------------------------------------------------------
// Helper functions
// ---------------------------------------------------------------------------

#ifdef ENABLE_TESTING
#define BM_LINUX_STATIC
#else
#define BM_LINUX_STATIC static
#endif

/// Build a BmIpAddr from a 32-bit prefix and 64-bit node_id.
/// All bytes are written in network (big-endian) order so that
/// ip_to_nodeid() from util.h can reverse the conversion.
BM_LINUX_STATIC void nodeid_to_ip(BmIpAddr *out, uint32_t prefix, uint64_t id) {
  memset(out, 0, sizeof(*out));
  out->addr[0] = (uint8_t)(prefix >> 24);
  out->addr[1] = (uint8_t)(prefix >> 16);
  out->addr[2] = (uint8_t)(prefix >> 8);
  out->addr[3] = (uint8_t)(prefix);
  /* bytes 4-7 are zero */
  uint32_t hi = (uint32_t)(id >> 32);
  uint32_t lo = (uint32_t)(id & 0xFFFFFFFF);
  out->addr[8] = (uint8_t)(hi >> 24);
  out->addr[9] = (uint8_t)(hi >> 16);
  out->addr[10] = (uint8_t)(hi >> 8);
  out->addr[11] = (uint8_t)(hi);
  out->addr[12] = (uint8_t)(lo >> 24);
  out->addr[13] = (uint8_t)(lo >> 16);
  out->addr[14] = (uint8_t)(lo >> 8);
  out->addr[15] = (uint8_t)(lo);
}

/// Format a BmIpAddr as a compressed IPv6 string (RFC 5952 :: compression).
/// Output buffer must be at least 40 bytes.
BM_LINUX_STATIC void format_ipv6(char *out, const BmIpAddr *addr) {
  uint16_t w[8];
  for (int i = 0; i < 8; i++) {
    w[i] = ((uint16_t)addr->addr[i * 2] << 8) | addr->addr[i * 2 + 1];
  }
  /* Find longest run of consecutive zero words (>= 2) for :: compression. */
  int zs = -1, zl = 0, cs = -1, cl = 0;
  for (int i = 0; i < 8; i++) {
    if (w[i] == 0) {
      if (cs < 0) {
        cs = i;
        cl = 0;
      }
      if (++cl > zl) {
        zs = cs;
        zl = cl;
      }
    } else {
      cs = -1;
      cl = 0;
    }
  }
  if (zl < 2) {
    zs = -1; /* only compress runs of 2+ */
  }
  char *p = out;
  bool need_colon = false;
  for (int i = 0; i < 8;) {
    if (i == zs) {
      p += sprintf(p, "::");
      i += zl;
      need_colon = false;
      continue;
    }
    if (need_colon) {
      *p++ = ':';
    }
    p += sprintf(p, "%x", w[i]);
    need_colon = true;
    i++;
  }
  *p = '\0';
}

/// RFC 2460 IPv6 pseudo-header checksum.  One's complement sum of:
///   16-byte src + 16-byte dst + 32-bit length + (24 zero bits + 8-bit
///   next_header) + upper-layer data.  Returns one's complement of the sum.
/// Must produce the same value as lwIP's ip6_chksum_pseudo for BCMP
/// checksum validation to work.
BM_LINUX_STATIC uint16_t ipv6_pseudo_checksum(const BmIpAddr *src,
                                              const BmIpAddr *dst,
                                              uint8_t next_header,
                                              uint32_t length,
                                              const void *data) {
  uint32_t sum = 0;
  const uint8_t *p;
  /* Source address (16 bytes). */
  p = src->addr;
  for (int i = 0; i < 16; i += 2) {
    sum += ((uint16_t)p[i] << 8) | p[i + 1];
  }
  /* Destination address (16 bytes). */
  p = dst->addr;
  for (int i = 0; i < 16; i += 2) {
    sum += ((uint16_t)p[i] << 8) | p[i + 1];
  }
  /* Upper-layer packet length (32-bit, network order). */
  sum += (length >> 16) & 0xFFFF;
  sum += length & 0xFFFF;
  /* Next header (preceded by 3 zero bytes — only the byte contributes). */
  sum += next_header;
  /* Upper-layer data. */
  p = (const uint8_t *)data;
  for (uint32_t i = 0; i + 1 < length; i += 2) {
    sum += ((uint16_t)p[i] << 8) | p[i + 1];
  }
  if (length & 1) {
    sum += (uint16_t)p[length - 1] << 8;
  }
  /* Fold carry bits and return one's complement. */
  while (sum >> 16) {
    sum = (sum & 0xFFFF) + (sum >> 16);
  }
  return ntohs((uint16_t)~sum);
}

/// Derive a 6-byte locally-administered MAC address from node_id.
/// Bit 1 of byte 0 is set (locally administered), bit 0 cleared (unicast).
BM_LINUX_STATIC void mac_from_nodeid(uint8_t *mac, uint64_t id) {
  mac[0] = (uint8_t)(id >> 40) | 0x02; /* set locally-administered bit */
  mac[0] &= (uint8_t)~0x01;            /* clear multicast bit */
  mac[1] = (uint8_t)(id >> 32);
  mac[2] = (uint8_t)(id >> 24);
  mac[3] = (uint8_t)(id >> 16);
  mac[4] = (uint8_t)(id >> 8);
  mac[5] = (uint8_t)(id);
}

/// Build an IPv6 multicast-mapped MAC: 33:33 followed by the last 4 bytes
/// of the IPv6 destination address.
BM_LINUX_STATIC void multicast_mac_from_ipv6(uint8_t *mac,
                                             const BmIpAddr *dst) {
  mac[0] = 0x33;
  mac[1] = 0x33;
  mac[2] = dst->addr[12];
  mac[3] = dst->addr[13];
  mac[4] = dst->addr[14];
  mac[5] = dst->addr[15];
}

/// Return true if the address is any IPv6 multicast (first byte 0xFF).
BM_LINUX_STATIC bool is_multicast(const BmIpAddr *addr) {
  return addr && addr->addr[0] == 0xFF;
}

// ---------------------------------------------------------------------------
// Frame header sizes
// ---------------------------------------------------------------------------

/// Ethernet (14) + IPv6 (40) header sizes.
#define ETH_HDR_LEN 14
#define IPV6_HDR_LEN 40
#define FRAME_HDR_LEN (ETH_HDR_LEN + IPV6_HDR_LEN)

// ---------------------------------------------------------------------------
// Packet accessor callbacks — registered via packet_init() so the BCMP
// packet processor can extract fields from a LinuxLayout.
// ---------------------------------------------------------------------------

BM_LINUX_STATIC void *message_get_data(void *payload) {
  return ((LinuxLayout *)payload)->data;
}

BM_LINUX_STATIC BmIpAddr *message_get_src_ip(void *payload) {
  return (BmIpAddr *)((LinuxLayout *)payload)->src;
}

BM_LINUX_STATIC BmIpAddr *message_get_dst_ip(void *payload) {
  return (BmIpAddr *)((LinuxLayout *)payload)->dst;
}

BM_LINUX_STATIC uint16_t message_get_checksum(void *payload, uint32_t size) {
  LinuxLayout *layout = (LinuxLayout *)payload;

  return ipv6_pseudo_checksum(layout->src, layout->dst, ip_proto_bcmp, size,
                              layout->data);
}

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------

BmErr bm_ip_init(void) {
  uint64_t id = node_id();

  /* Build addresses from node_id. */
  nodeid_to_ip(&CTX.ll_addr, 0xFE800000, id);
  nodeid_to_ip(&CTX.unicast_addr, 0xFD000000, id);

  /* ff03::1 global multicast. */
  memset(&CTX.multicast_addr, 0, sizeof(BmIpAddr));
  CTX.multicast_addr.addr[0] = 0xFF;
  CTX.multicast_addr.addr[1] = 0x03;
  CTX.multicast_addr.addr[15] = 0x01;

  /* Pre-format address strings for bm_ip_get_str. */
  format_ipv6(CTX.ip_str[0], &CTX.ll_addr);
  format_ipv6(CTX.ip_str[1], &CTX.unicast_addr);

  /* UDP list starts empty (zero-initialized). */
  memset(&CTX.udp_list, 0, sizeof(CTX.udp_list));

  /* Register BCMP packet accessor callbacks. */
  return packet_init(message_get_src_ip, message_get_dst_ip, message_get_data,
                     message_get_checksum);
}

void *bm_l2_new(uint32_t size) {
  LinuxBuf *b = (LinuxBuf *)bm_malloc(sizeof(LinuxBuf) + size);
  if (b) {
    b->ref = 1;
    b->alloc_size = size;
    b->len = size;
  }
  return b;
}

void *bm_l2_get_payload(void *buf) {
  if (!buf) {
    return NULL;
  }
  return ((LinuxBuf *)buf)->payload;
}

void bm_l2_tx_prep(void *buf, uint32_t size) {
  (void)size; /* unused — matches lwIP which ignores this param */
  if (buf) {
    __sync_add_and_fetch(&((LinuxBuf *)buf)->ref, 1);
  }
}

void bm_l2_free(void *buf) {
  if (buf && __sync_sub_and_fetch(&((LinuxBuf *)buf)->ref, 1) == 0) {
    bm_free(buf);
  }
}

BmErr bm_l2_submit(void *buf, uint32_t size) {
  if (!buf || size < FRAME_HDR_LEN) {
    return BmEBADMSG;
  }

  uint8_t *frame = (uint8_t *)bm_l2_get_payload(buf);
  if (!frame) {
    return BmEBADMSG;
  }

  /* --- Ethernet header validation --- */
  if (uint8_to_uint16(&frame[12]) != ethernet_type_ipv6) {
    return BmEBADMSG;
  }

  /* --- IPv6 header fields --- */
  uint8_t next_header = frame[20];
  uint16_t payload_length = uint8_to_uint16(&frame[18]);
  BmIpAddr src_addr;
  memcpy(src_addr.addr, &frame[22], sizeof(BmIpAddr));

  if (next_header == ip_proto_bcmp) {
    /* --- BCMP RX path --- */
    if (size < FRAME_HDR_LEN + sizeof(BcmpHeader)) {
      return BmEBADMSG;
    }

    BmIpAddr *src_copy = (BmIpAddr *)bm_malloc(sizeof(BmIpAddr));
    BmIpAddr *dst_copy = (BmIpAddr *)bm_malloc(sizeof(BmIpAddr));
    void *data_copy = bm_malloc(payload_length);
    LinuxLayout *layout = (LinuxLayout *)bm_malloc(sizeof(LinuxLayout));

    if (!src_copy || !dst_copy || !data_copy || !layout) {
      bm_free(src_copy);
      bm_free(dst_copy);
      bm_free(data_copy);
      bm_free(layout);
      return BmENOMEM;
    }

    memcpy(src_copy, &frame[22], sizeof(BmIpAddr));
    memcpy(dst_copy, &frame[38], sizeof(BmIpAddr));
    memcpy(data_copy, frame + FRAME_HDR_LEN, payload_length);
    *layout = (LinuxLayout){
        .data = data_copy,
        .data_size = payload_length,
        .src = src_copy,
        .dst = dst_copy,
    };

    BcmpQueueItem item = {BcmpEventRx, (void *)layout, payload_length};
    if (bm_queue_send(bcmp_get_queue(), &item, 0) != BmOK) {
      bm_free(src_copy);
      bm_free(dst_copy);
      bm_free(data_copy);
      bm_free(layout);
      return BmEAGAIN;
    }

    /* Success — we own the buf now, free it. */
    bm_l2_free(buf);
    return BmOK;

  } else if (next_header == ip_proto_udp) {
    /* --- UDP RX path --- */
    /* 54 (frame header) + 8 (UDP header) = 62 minimum */
    if (size < FRAME_HDR_LEN + 8) {
      return BmEBADMSG;
    }

    uint16_t src_port = uint8_to_uint16(&frame[54]);
    uint16_t dst_port = uint8_to_uint16(&frame[56]);
    uint16_t udp_length = uint8_to_uint16(&frame[58]);
    uint16_t udp_payload_len = (udp_length >= 8) ? (udp_length - 8) : 0;

    void *udp_buf = bm_l2_new(udp_payload_len);
    if (!udp_buf) {
      return BmENOMEM;
    }

    if (udp_payload_len > 0) {
      memcpy(bm_l2_get_payload(udp_buf), frame + FRAME_HDR_LEN + 8,
             udp_payload_len);
    }

    UdpCb *cb = NULL;
    if (ll_get_item(&CTX.udp_list, (uint32_t)dst_port, (void **)&cb) == BmOK &&
        cb != NULL) {
      cb->udp_cb(src_port, udp_buf, ip_to_nodeid(&src_addr), udp_payload_len);
    } else {
      /* No listener — discard the copied payload. */
      bm_l2_free(udp_buf);
    }

    /* Success — we own the original buf, free it. */
    bm_l2_free(buf);
    return BmOK;
  }

  /* Unknown next_header — don't free buf (caller will). */
  return BmEBADMSG;
}

BmErr bm_l2_set_netif(bool up) {
  CTX.link_up = up;
  return BmOK;
}

const char *bm_ip_get_str(uint8_t idx) {
  if (idx < 2) {
    return CTX.ip_str[idx];
  }
  return "::";
}

const BmIpAddr *bm_ip_get(uint8_t idx) {
  if (idx == 0) {
    return &CTX.ll_addr;
  } else if (idx == 1) {
    return &CTX.unicast_addr;
  }
  return NULL;
}

void bm_ip_rx_cleanup(void *payload) {
  if (payload) {
    LinuxLayout *layout = (LinuxLayout *)payload;
    if (layout->data) {
      bm_free(layout->data);
    }
    if (layout->src) {
      bm_free((void *)layout->src);
    }
    if (layout->dst) {
      bm_free((void *)layout->dst);
    }
    bm_free(layout);
  }
}

void *bm_ip_tx_new(const BmIpAddr *dst, uint32_t size) {
  LinuxLayout *layout = NULL;
  if (dst) {
    void *data = bm_malloc(size);
    layout = (LinuxLayout *)bm_malloc(sizeof(LinuxLayout));
    if (data && layout) {
      *layout = (LinuxLayout){
          .data = data,
          .data_size = size,
          .src = &CTX.ll_addr,
          .dst = dst,
      };
    } else {
      bm_free(data);
      bm_free(layout);
      layout = NULL;
    }
  }
  return (void *)layout;
}

BmErr bm_ip_tx_copy(void *payload, const void *data, uint32_t size,
                    uint32_t offset) {
  BmErr err = BmEINVAL;
  if (payload && data) {
    LinuxLayout *layout = (LinuxLayout *)payload;
    memcpy((uint8_t *)layout->data + offset, data, size);
    err = BmOK;
  }
  return err;
}

BmErr bm_ip_tx_perform(void *payload, const BmIpAddr *dst) {
  BmErr err = BmEINVAL;
  if (!payload) {
    return err;
  }

  LinuxLayout *layout = (LinuxLayout *)payload;
  const BmIpAddr *effective_dst = dst ? dst : layout->dst;
  uint32_t data_size = layout->data_size;
  uint32_t frame_size = FRAME_HDR_LEN + data_size;

  void *l2_buf = bm_l2_new(frame_size);
  if (!l2_buf) {
    return BmENOMEM;
  }

  uint8_t *frame = (uint8_t *)bm_l2_get_payload(l2_buf);

  /* --- Ethernet header (14 bytes) --- */
  if (is_multicast(effective_dst)) {
    multicast_mac_from_ipv6(frame, effective_dst);
  } else {
    memset(frame, 0xFF, 6); /* broadcast */
  }
  mac_from_nodeid(frame + 6, node_id());
  frame[12] = (uint8_t)(ethernet_type_ipv6 >> 8);
  frame[13] = (uint8_t)(ethernet_type_ipv6);

  /* --- IPv6 header (40 bytes at offset 14) --- */
  uint8_t *ip = frame + ETH_HDR_LEN;
  ip[0] = 0x60; /* version 6, traffic class 0 */
  ip[1] = 0x00;
  ip[2] = 0x00;
  ip[3] = 0x00; /* flow label 0 */
  ip[4] = (uint8_t)(data_size >> 8);
  ip[5] = (uint8_t)(data_size); /* payload length */
  ip[6] = ip_proto_bcmp;        /* next header */
  ip[7] = 64;                   /* hop limit */
  memcpy(ip + 8, layout->src->addr, 16);
  memcpy(ip + 24, effective_dst->addr, 16);

  /* --- BCMP payload (at offset 54) --- */
  memcpy(frame + FRAME_HDR_LEN, layout->data, data_size);

  err = bm_l2_link_output(l2_buf, frame_size);
  if (err != BmOK) {
    bm_l2_free(l2_buf);
  }
  return err;
}

void bm_ip_tx_cleanup(void *payload) {
  if (payload) {
    LinuxLayout *layout = (LinuxLayout *)payload;
    bm_free(layout->data);
    bm_free(layout);
  }
}

void *bm_udp_bind_port(const BmIpAddr *addr, uint16_t port,
                       BmErr (*cb)(uint16_t, void *, uint64_t, uint32_t)) {
  (void)addr; /* multicast group — L2 handles forwarding, no MLD needed */

  LinuxUdpPcb *pcb = NULL;
  LLItem *item = NULL;

  if (cb) {
    pcb = (LinuxUdpPcb *)bm_malloc(sizeof(LinuxUdpPcb));
    UdpCb udp_cb = {cb};
    item = ll_create_item(NULL, &udp_cb, sizeof(udp_cb), (uint32_t)port);

    if (pcb && item && ll_item_add(&CTX.udp_list, item) == BmOK) {
      pcb->port = port;
    } else {
      bm_free(pcb);
      if (item) {
        ll_delete_item(item);
      }
      pcb = NULL;
    }
  }
  return (void *)pcb;
}

void *bm_udp_new(uint32_t size) { return bm_l2_new(size); }

void *bm_udp_get_payload(void *buf) { return bm_l2_get_payload(buf); }

BmErr bm_udp_reference_update(void *buf) {
  BmErr err = BmEINVAL;
  if (buf) {
    bm_l2_tx_prep(buf, 0);
    err = BmOK;
  }
  return err;
}

void bm_udp_cleanup(void *buf) { bm_l2_free(buf); }

#define UDP_HDR_LEN 8

BmErr bm_udp_tx_perform(void *pcb, void *buf, uint32_t size,
                        const BmIpAddr *dest_addr, uint16_t port) {
  BmErr err = BmEINVAL;
  if (!pcb || !buf || !dest_addr) {
    return err;
  }

  LinuxUdpPcb *udp_pcb = (LinuxUdpPcb *)pcb;
  uint32_t udp_total = UDP_HDR_LEN + size; /* UDP header + payload */
  uint32_t frame_size = FRAME_HDR_LEN + udp_total;

  void *l2_buf = bm_l2_new(frame_size);
  if (!l2_buf) {
    return BmENOMEM;
  }

  uint8_t *frame = (uint8_t *)bm_l2_get_payload(l2_buf);

  /* --- Ethernet header (14 bytes) --- */
  if (is_multicast(dest_addr)) {
    multicast_mac_from_ipv6(frame, dest_addr);
  } else {
    memset(frame, 0xFF, 6); /* broadcast */
  }
  mac_from_nodeid(frame + 6, node_id());
  frame[12] = (uint8_t)(ethernet_type_ipv6 >> 8);
  frame[13] = (uint8_t)(ethernet_type_ipv6);

  /* --- IPv6 header (40 bytes at offset 14) --- */
  uint8_t *ip = frame + ETH_HDR_LEN;
  ip[0] = 0x60;
  ip[1] = 0x00;
  ip[2] = 0x00;
  ip[3] = 0x00;
  ip[4] = (uint8_t)(udp_total >> 8);
  ip[5] = (uint8_t)(udp_total); /* payload length = UDP hdr + data */
  ip[6] = ip_proto_udp;         /* next header */
  ip[7] = 64;                   /* hop limit */
  memcpy(ip + 8, CTX.ll_addr.addr, 16);
  memcpy(ip + 24, dest_addr->addr, 16);

  /* --- UDP header (8 bytes at offset 54) --- */
  uint8_t *udp = frame + FRAME_HDR_LEN;
  udp[0] = (uint8_t)(udp_pcb->port >> 8);
  udp[1] = (uint8_t)(udp_pcb->port); /* src port */
  udp[2] = (uint8_t)(port >> 8);
  udp[3] = (uint8_t)(port); /* dst port */
  udp[4] = (uint8_t)(udp_total >> 8);
  udp[5] = (uint8_t)(udp_total); /* length */
  udp[6] = 0;
  udp[7] = 0; /* zeroed before checksum computation */

  /* --- UDP payload (at offset 62) --- */
  if (size > 0) {
    memcpy(frame + FRAME_HDR_LEN + UDP_HDR_LEN, bm_udp_get_payload(buf), size);
  }

  /* --- UDP checksum (mandatory for IPv6, RFC 2460 §8.1) --- */
  {
    BmIpAddr src_addr;
    memcpy(src_addr.addr, ip + 8, 16);
    uint16_t cksum = ipv6_pseudo_checksum(&src_addr, dest_addr, ip_proto_udp,
                                          udp_total, udp);
    udp[6] = (uint8_t)(cksum >> 8);
    udp[7] = (uint8_t)(cksum);
  }

  err = bm_l2_link_output(l2_buf, frame_size);
  if (err != BmOK) {
    bm_l2_free(l2_buf);
  }
  return err;
}

void bm_ip_buf_shrink(void *buf, uint32_t size) {
  if (buf) {
    ((LinuxBuf *)buf)->len = size;
  }
}
