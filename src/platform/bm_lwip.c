#include "bcmp.h"
#include "bm_ip.h"
#include "bm_os.h"
#include "lwip/inet_chksum.h"
#include "lwip/raw.h"
#include "packet.h"
#include "util.h"
#include <string.h>

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

static struct LwipCtx CTX;

/*!
 @brief Obtain Data From LwipLayout Abstraction

 @param payload abstract payload handled by packet interface

 @return pointer to payload
 */
static void *message_get_data(void *payload) {
  LwipLayout *ret = (LwipLayout *)payload;
  return (void *)ret->pbuf->payload;
}

/*!
 @brief Get Src IP Address From LwipLayout Abstraction

 @param payload

 @return pointer to source ip address
 */
static void *message_get_src_ip(void *payload) {
  LwipLayout *ret = (LwipLayout *)payload;
  return (void *)ret->src;
}

/*!
 @brief Get Src IP Address From LwipLayout Abstraction

 @param payload abstract payload handled by packet interface

 @return pointer to destination ip address
 */
static void *message_get_dst_ip(void *payload) {
  LwipLayout *ret = (LwipLayout *)payload;
  return (void *)ret->dst;
}

/*!
 @brief Obtain Checksum From Payload Of LwipLayout Item

 @param payload abstract payload handled by packet interface
 @param size size of the payload

 @return calculated checksum of message
 */
static uint16_t message_get_checksum(void *payload, uint32_t size) {
  uint16_t ret = UINT16_MAX;
  LwipLayout *data = (LwipLayout *)payload;

  ret = ip6_chksum_pseudo(data->pbuf, IpProtoBcmp, size,
                          (ip_addr_t *)message_get_src_ip(payload),
                          (ip_addr_t *)message_get_dst_ip(payload));

  return ret;
}

/*!
  @brief LWIP raw_recv Callback For BCMP Packets

  @details Packet is added to main BCMP queue for processing

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

  // Don't eat the packet unless we process it
  uint8_t rval = 0;
  BmQueue queue = (BmQueue)arg;

  if (pbuf->tot_len >= (PBUF_IP_HLEN + sizeof(BcmpHeader))) {
    // Grab a pointer to the ip6 header so we can get the packet destination
    struct ip6_hdr *ip6_hdr = (struct ip6_hdr *)pbuf->payload;

    if (pbuf_remove_header(pbuf, PBUF_IP_HLEN) != 0) {
      //  Restore original packet
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

      BcmpQueueItem item = {BcmpEventRx, (void *)layout, p_ref->len};
      if (bm_queue_send(queue, &item, 0) != BmOK) {
        printf("Error sending to Queue\n");
        pbuf_free(pbuf);
      }

      // Eat the packet
      rval = 1;
    }
  }

  return rval;
}

/*!
 @brief Initialize Network Layer For Bristlemouth Stack

 @param queue BCMP queue the received messages will enqueue

 @return BmOK on success
 @return BmErr on failure
 */
BmErr bm_ip_init(void *queue) {
  BmErr err = BmENOMEM;
  CTX.netif = &netif;
  CTX.pcb = raw_new(IpProtoBcmp);
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

/*!
 @brief Cleanup Data From A Received Message

 @details This should be called after the queued message has been handled and
          processed.

 @param payload abstracted payload object to be handled
 */
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

/*!
 @brief Create A New Network Layer Object

 @details A destination address is passed into this API because there may be
          some operations that require this before sending the packet.
          The abstraction is responsible for determining the source IP address
          and assigning it here to its associated abstraction object

 @param dst destination IP address
 @param size sizeo of object to create

 @return pointer to created abstraction object
 */
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

/*!
 @brief Copy Data To Payload

 @param payload created object from bm_ip_tx_new
 @param data data to be copied
 @param size size of data to be copied
 @param offset offset from beginning of buffer to copy data

 @return BmOK on success
 @return BmErr on failure
 */
BmErr bm_ip_tx_copy(void *payload, const void *data, uint32_t size,
                    uint32_t offset) {
  BmErr err = BmEINVAL;
  LwipLayout *layout = NULL;

  if (payload && data) {
    err = BmOK;
    layout = (LwipLayout *)payload;
    memcpy(layout->pbuf->payload + offset, data, size);
  }

  return err;
}

/*!
 @brief Perform A Network Layer Transmission

 @details The destination address is optional, if NULL the address passed
          into bm_ip_tx_new will be utilized

 @param payload abstracted payload to be 
 @param dst destination IP address to send the message to

 @return 
 */
BmErr bm_ip_tx_perform(void *payload, const void *dst) {
  BmErr err = BmEINVAL;
  LwipLayout *layout = NULL;
  if (payload) {
    layout = (LwipLayout *)payload;
    layout->dst = dst == NULL ? layout->dst : (const ip_addr_t *)dst;
    err = raw_sendto_if_src(CTX.pcb, layout->pbuf, layout->dst, CTX.netif,
                            layout->src) == ERR_OK
              ? BmOK
              : BmEBADMSG;
  }
  return err;
}

/*!
 @brief Cleanup Abstraction Object After A Transmission

 @param payload abstracted payload object to be handled
 */
void bm_ip_tx_cleanup(void *payload) {
  LwipLayout *layout = NULL;
  if (payload) {
    layout = (LwipLayout *)payload;
    pbuf_free(layout->pbuf);
    bm_free(layout);
  }
}
