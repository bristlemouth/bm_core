#include "bm_ip.h"
#include "bm_os.h"
#include "lwip/inet_chksum.h"
#include "lwip/raw.h"
#include "messages.h"
#include "packet.h"
#include "util.h"
#include <string.h>

//TODO: this will have to be placed in a header somewhere
#define IP_PROTO_BCMP (0xBC)

typedef enum {
  BCMP_EVT_RX,
  BCMP_EVT_HEARTBEAT,
} BcmpQueueType;
typedef struct {
  BcmpQueueType type;
  void *data;
  uint32_t size;
} BcmpQueueItem;

//TODO: this will have to be moved as we go along
extern struct netif netif;
struct LwipCtx {
  struct netif *netif;
  struct raw_pcb *pcb;
};
typedef struct {
  struct pbuf *pbuf;
  const ip_addr_t *src;
  const ip_addr_t *dst;
} LwipLayout;

struct LwipCtx CTX;

static void *message_get_data(void *payload) {
  LwipLayout *ret = (LwipLayout *)payload;
  return (void *)ret->pbuf->payload;
}

static void *message_get_src_ip(void *payload) {
  LwipLayout *ret = (LwipLayout *)payload;
  return (void *)ret->src;
}

static void *message_get_dst_ip(void *payload) {
  LwipLayout *ret = (LwipLayout *)payload;
  return (void *)ret->dst;
}

static uint16_t message_get_checksum(void *payload, uint32_t size) {
  uint16_t ret = UINT16_MAX;
  LwipLayout *data = (LwipLayout *)payload;

  ret = ip6_chksum_pseudo(data->pbuf, IP_PROTO_BCMP, size,
                          (ip_addr_t *)message_get_src_ip(payload),
                          (ip_addr_t *)message_get_dst_ip(payload));

  return ret;
}

/*!
  lwip raw recv callback for BCMP packets. Called from lwip task.
  Packet is added to main BCMP queue for processing

  \param *arg unused
  \param *pcb unused
  \param *pbuf packet buffer
  \param *src packet sender address
  \return 1 if the packet was consumed, 0 otherwise (someone else will take it)
*/
static uint8_t bcmp_recv(void *arg, struct raw_pcb *pcb, struct pbuf *pbuf,
                         const ip_addr_t *src) {
  (void)arg;
  (void)pcb;

  // don't eat the packet unless we process it
  uint8_t rval = 0;
  BmQueue queue = (BmQueue)arg;

  if (pbuf->tot_len >= (PBUF_IP_HLEN + sizeof(BcmpHeader))) {
    // Grab a pointer to the ip6 header so we can get the packet destination
    struct ip6_hdr *ip6_hdr = (struct ip6_hdr *)pbuf->payload;

    if (pbuf_remove_header(pbuf, PBUF_IP_HLEN) != 0) {
      //  restore original packet
      pbuf_add_header(pbuf, PBUF_IP_HLEN);
    } else {

      // Make a copy of the IP address since we'll be modifying it later when we
      // remove the src/dest ports (and since it might not be in the pbuf so someone
      // else is managing that memory)
      struct pbuf *p_ref = pbuf_alloc(PBUF_IP, pbuf->len, PBUF_RAM);
      ip_addr_t *src_ref = (ip_addr_t *)bm_malloc(sizeof(ip_addr_t));
      ip_addr_t *dst_ref = (ip_addr_t *)bm_malloc(sizeof(ip_addr_t));
      LwipLayout *layout = (LwipLayout *)bm_malloc(sizeof(LwipLayout));
      pbuf_copy(p_ref, pbuf);
      memcpy(dst_ref, ip6_hdr->dest.addr, sizeof(ip_addr_t));
      memcpy(src_ref, src, sizeof(ip_addr_t));
      *layout = (LwipLayout){p_ref, src_ref, dst_ref};

      BcmpQueueItem item = {BCMP_EVT_RX, (void *)layout, p_ref->len};
      if (bm_queue_send(queue, &item, 0) != BmOK) {
        printf("Error sending to Queue\n");
        pbuf_free(pbuf);
      }

      // eat the packet
      rval = 1;
    }
  }

  return rval;
}

BmErr bm_ip_init(void *queue) {
  BmErr err = BmENOMEM;
  CTX.netif = &netif;
  CTX.pcb = raw_new(IP_PROTO_BCMP);
  if (CTX.pcb) {
    raw_recv(CTX.pcb, bcmp_recv, queue);
    err = raw_bind(CTX.pcb, IP_ADDR_ANY) == ERR_OK ? BmOK : BmEACCES;
    if (err == BmOK) {
      err = packet_init(message_get_src_ip, message_get_dst_ip,
                        message_get_data, message_get_checksum);
    }
  }
  return err;
}

void bm_ip_rx_cleanup(void *payload) {
  LwipLayout *layout = NULL;
  if (payload) {
    layout = (LwipLayout *)payload;
    if (layout->pbuf) {
      pbuf_free(layout->pbuf);
    }
    if (layout->dst) {
      bm_free((void *)layout->dst);
    }
    if (layout->src) {
      bm_free((void *)layout->src);
    }
    bm_free(layout);
  }
}

void *bm_ip_tx_new(const void *dst, uint32_t size) {
  struct pbuf *pbuf = NULL;
  LwipLayout *layout = NULL;
  const ip_addr_t *src = netif_ip_addr6(CTX.netif, 0);

  if (dst) {
    pbuf = pbuf_alloc(PBUF_IP, size, PBUF_RAM);
    layout = (LwipLayout *)bm_malloc(sizeof(LwipLayout));
    *layout = (LwipLayout){pbuf, src, (const ip_addr_t *)dst};
  }

  return (void *)layout;
}

BmErr bm_ip_tx_perform(void *payload) {
  BmErr err = BmEINVAL;
  LwipLayout *layout = NULL;
  if (payload) {
    layout = (LwipLayout *)payload;
    err = raw_sendto_if_src(CTX.pcb, layout->pbuf, layout->dst, CTX.netif,
                            layout->src) == ERR_OK
              ? BmOK
              : BmEBADMSG;
  }
  return err;
}

void bm_ip_tx_cleanup(void *payload) {
  LwipLayout *layout = NULL;
  if (payload) {
    layout = (LwipLayout *)payload;
    pbuf_free(layout->pbuf);
    bm_free(layout);
  }
}
