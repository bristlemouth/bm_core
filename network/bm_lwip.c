#include "bcmp.h"
#include "bm_config.h"
#include "bm_ip.h"
#include "bm_os.h"
#include "device.h"
#include "l2.h"
#include "ll.h"
#include "lwip/ethip6.h"
#include "lwip/inet.h"
#include "lwip/inet_chksum.h"
#include "lwip/init.h"
#include "lwip/mld6.h"
#include "lwip/prot/ethernet.h"
#include "lwip/raw.h"
#include "lwip/snmp.h"
#include "lwip/tcpip.h"
#include "lwip/udp.h"
#include "packet.h"
#include "util.h"
#include <string.h>

#define netif_link_speed_bps 10000000
#define ifname0 'b'
#define ifname1 'm'
#define ethernet_mtu 1500

static struct netif netif;

typedef struct {
  BmErr (*udp_cb)(void *, uint64_t, uint32_t);
} UdpCb;

struct LwipCtx {
  struct netif *netif;
  struct raw_pcb *raw_pcb;
  LL udp_list;
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

  ret = ip6_chksum_pseudo(data->pbuf, ip_proto_bcmp, size,
                          (ip_addr_t *)message_get_src_ip(payload),
                          (ip_addr_t *)message_get_dst_ip(payload));

  return ret;
}

/*!
  @brief LWIP raw_recv Callback For BCMP Packets

  @details Packet is added to main BCMP queue for processing

  @param *arg unused
  @param *pcb unused
  @param *pbuf packet buffer
  @param *src packet sender address
  @return 1 if the packet was consumed, 0 otherwise (someone else will take it)
*/
static uint8_t ip_recv(void *arg, struct raw_pcb *pcb, struct pbuf *pbuf,
                       const ip_addr_t *src) {
  (void)arg;
  (void)pcb;

  // Don't eat the packet unless we process it
  uint8_t rval = 0;
  BmQueue queue = bcmp_get_queue();

  if (pbuf->tot_len >= (PBUF_IP_HLEN + sizeof(BcmpHeader))) {
    // Grab a pointer to the ip6 header so we can get the packet destination
    struct ip6_hdr *ip6_hdr = (struct ip6_hdr *)pbuf->payload;

    if (pbuf_remove_header(pbuf, PBUF_IP_HLEN) != 0) {
      //  Restore original packet
      pbuf_add_header(pbuf, PBUF_IP_HLEN);
    }

    // Make a copy of the IP address since we'll be modifying it later when we
    // remove the src/dest ports (and since it might not be in the pbuf so someone
    // else is managing that memory)
    ip_addr_t *src_ref = (ip_addr_t *)bm_malloc(sizeof(ip_addr_t));
    ip_addr_t *dst_ref = (ip_addr_t *)bm_malloc(sizeof(ip_addr_t));
    LwipLayout *layout = (LwipLayout *)bm_malloc(sizeof(LwipLayout));
    memcpy(dst_ref, ip6_hdr->dest.addr, sizeof(ip_addr_t));
    memcpy(src_ref, src, sizeof(ip_addr_t));
    *layout = (LwipLayout){pbuf, src_ref, dst_ref};

    BcmpQueueItem item = {BcmpEventRx, (void *)layout, layout->pbuf->len};
    if (bm_queue_send(queue, &item, 0) != BmOK) {
      bm_debug("Error sending to Queue\n");
      pbuf_free(pbuf);
      bm_free(src_ref);
      bm_free(dst_ref);
      bm_free(layout);
    }

    // Eat the packet
    rval = 1;
  }

  return rval;
}

/*!
  @brief Lwip UDP Receive Callback

  @details This requests an item from a linked list in accordance to the pcb
           of the interface and invokes the associated callback this allows for
           multiple pcbs to be utilized, example middleware and stress

  @param *arg unused
  @param *pcb UDP PCB
  @param *pbuf data pbuf
  @param *addr Source address
  @param port unused
*/
static void udp_recv_cb(void *arg, struct udp_pcb *pcb, struct pbuf *pbuf,
                        const ip_addr_t *addr, u16_t port) {

  (void)arg;
  (void)port;
  UdpCb *cb = NULL;
  if (ll_get_item(&CTX.udp_list, (uint32_t)pcb, (void **)&cb) == BmOK &&
      cb != NULL) {
    cb->udp_cb(pbuf, ip_to_nodeid((void *)addr), pbuf->len);
  }
}

/*!
  @brief LWIP Callback For Transmitting Messages From LWIP Stack

  @details This calls bm_l2_link_output in bm_l2 module

  @param netif unused
  @param pbuf lwip buffer to be transmitted

  @return ERR_OK on success
  @return ERR_IF on failure
*/
static err_t link_output(struct netif *netif, struct pbuf *pbuf) {
  (void)netif;

  return bm_l2_link_output(pbuf, pbuf->len) == BmOK ? ERR_OK : ERR_IF;
}

/*!
  @brief Initialize L2 Network Interface Items

  @details This is passed in as a callback to netif_add by lwip

  @param netif network interface to intialize items specific to L2 for

  @return ERR_OK
*/
static err_t bm_l2_netif_init(struct netif *netif) {
  MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, netif_link_speed_bps);

  netif->name[0] = ifname0;
  netif->name[1] = ifname1;
  netif->output_ip6 = ethip6_output;
  netif->linkoutput = link_output;

  netif->mtu = ethernet_mtu;
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

  return ERR_OK;
}

/*!
  @brief Thread Safe Wrapper For udp_sendto_if

  @param pcb protocol control block to send data to
  @param pbuf pbuf to send data to
  @param dst_ip destination IP address to send data to
  @param dst_port destination port to send data to
  @param netif network interface

  @return ERR_OK on success
  @return ERR_X on failure
 */
static inline err_t safe_udp_sendto_if(struct udp_pcb *pcb, struct pbuf *pbuf,
                                       const ip_addr_t *dst_ip, u16_t dst_port,
                                       struct netif *netif) {
  LOCK_TCPIP_CORE();
  err_t rval = udp_sendto_if(pcb, pbuf, dst_ip, dst_port, netif);
  UNLOCK_TCPIP_CORE();

  return rval;
}

/*!
 @brief Initialize Network Layer For Bristlemouth Stack

 @return BmOK on success
 @return BmErr on failure
*/
BmErr bm_ip_init(void) {
  BmErr err = BmENOMEM;
  err_t mld6_err;
  ip6_addr_t multicast_glob_addr;

  CTX.netif = &netif;

  tcpip_init(NULL, NULL);
  mac_address(CTX.netif->hwaddr, sizeof(CTX.netif->hwaddr));
  CTX.netif->hwaddr_len = sizeof(CTX.netif->hwaddr);
  inet6_aton("ff03::1", &multicast_glob_addr);

  // The documentation says to use tcpip_input if we are running with an OS
  // bm_l2_netif_init will be called from netif_add
  netif_add(CTX.netif, NULL, bm_l2_netif_init, tcpip_input);

  // Create IP addresses from node ID
  const uint64_t id = node_id();
  ip_addr_t unicast_addr = IPADDR6_INIT_HOST(
      0xFD000000, 0, (id >> 32) & 0xFFFFFFFF, id & 0xFFFFFFFF);

  ip_addr_t ll_addr = IPADDR6_INIT_HOST(0xFE800000, 0, (id >> 32) & 0xFFFFFFFF,
                                        id & 0xFFFFFFFF);

  // Generate IPv6 address using EUI-64 format derived from mac address
  netif_ip6_addr_set(CTX.netif, 0, ip_2_ip6(&ll_addr));
  netif_ip6_addr_set(CTX.netif, 1, ip_2_ip6(&unicast_addr));
  netif_set_up(CTX.netif);
  netif_ip6_addr_set_state(CTX.netif, 0, IP6_ADDR_VALID);
  netif_ip6_addr_set_state(CTX.netif, 1, IP6_ADDR_VALID);

  netif_set_link_up(CTX.netif);

  /* Add to relevant multicast groups */
  mld6_err = mld6_joingroup_netif(CTX.netif, &multicast_glob_addr);
  if (mld6_err != ERR_OK) {
    bm_debug("Could not join ff03::1\n");
  }

  CTX.raw_pcb = raw_new(ip_proto_bcmp);
  if (CTX.raw_pcb) {
    raw_recv(CTX.raw_pcb, ip_recv, NULL);
    err = raw_bind(CTX.raw_pcb, IP_ADDR_ANY) == ERR_OK ? BmOK : BmEACCES;
    if (err == BmOK) {
      err = packet_init(message_get_src_ip, message_get_dst_ip,
                        message_get_data, message_get_checksum);
    }
  }
  return err;
}

/*!
  @brief Allocate A New L2 Buffer

  @param size size of buffer to create in bytes

  @return pointer to created buffer
*/
void *bm_l2_new(uint32_t size) { return pbuf_alloc(PBUF_RAW, size, PBUF_RAM); }

/*!
  @brief Obtain The Raw Payload From An L2 Buffer

  @param buf L2 buffer created with bm_l2_new

  @return pointer to payload of L2 buffer
*/
void *bm_l2_get_payload(void *buf) {
  void *ret = NULL;
  if (buf) {
    ret = ((struct pbuf *)buf)->payload;
  }
  return ret;
}

/*!
  @brief Prepare to Transmit

  @details This allows for some logic to occur before a transmission

  @param buf buffer created with bm_l2_new
  @param size size of buffer to transmit up the stack in bytes
*/
void bm_l2_tx_prep(void *buf, uint32_t size) {
  (void)size;
  if (buf) {
    pbuf_ref((struct pbuf *)buf);
  }
}

/*!
  @brief Free An L2 Buffer Created With `bm_l2_new`

  @param buf buffer created with bm_l2_new
*/
void bm_l2_free(void *buf) { pbuf_free((struct pbuf *)buf); }

/*!
  @brief Submit A Buffer Up Bristlemouth Stack

  @details This shall handle transmitting the L2 (raw) data up to
           the bcmp and middleware implementations. L2 is responsible for
           freeing any memory that might have been allocated if this function
           returns an error.

  @param buf buffer created with bm_l2_new
  @param size size of buffer to submite up the stack in bytes

  @return 
*/
BmErr bm_l2_submit(void *buf, uint32_t size) {
  BmErr err = BmOK;
  (void)size;

  // We're using tcpip_input in the netif, which is thread safe, so no
  // need for additional locking
  if (CTX.netif->input((struct pbuf *)buf, CTX.netif) != ERR_OK) {
    err = BmEBADMSG;
    // Do not need to free memory here as it is handled in L2
  }

  return err;
}

/*!
  @brief Set the network interface up or down

  @param up true if setting the network interface up, false if setting it down.

  @return BmOK on success
  @return BmErr on failure
*/
BmErr bm_l2_set_netif(bool up) {
  if (up) {
    netif_set_link_up(CTX.netif);
    netif_set_up(CTX.netif);
  } else {
    netif_set_link_down(CTX.netif);
    netif_set_down(CTX.netif);
  }
  return BmOK;
}

/*!
  @brief Get IP Address As A String

  @param idx index to return IP address of,
             addresses should be as follows when setting up abstraction:
                 0. ll address
                 1. unicast address

  @return string of IP address in format XXXX::XXXX:XXXX:XXXX:XXXX
*/
const char *bm_ip_get_str(uint8_t idx) {
  //NOTE: returns ptr to static buffer; not reentrant! (from ip6addr_ntoa)
  return (ip6addr_ntoa(netif_ip6_addr(&netif, idx)));
}

/*!
  @brief Get IP Address

  @param idx index to return IP address of,
             addresses should be as follows when setting up abstraction:
                 0. ll address
                 1. unicast address

  @return Pointer to static IP address
*/
const void *bm_ip_get(uint8_t idx) {
  return (const void *)netif_ip6_addr(CTX.netif, idx);
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
    err = raw_sendto_if_src(CTX.raw_pcb, layout->pbuf, layout->dst, CTX.netif,
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

/*!
  @brief Bind A New Port To A UDP PCB

  @details This API should allow for multiple callbacks to be invoked from
           different modules that utilize UDP communication

  @param port port number to bind to pcb
  @param cb callback of specified pcb

  @return pointer to pcb if successful
  @return NULL if unsuccessful
*/
void *bm_udp_bind_port(uint16_t port, BmErr (*cb)(void *, uint64_t, uint32_t)) {

  struct udp_pcb *pcb = udp_new_ip_type(IPADDR_TYPE_V6);
  LLItem *item = NULL;
  UdpCb udp_cb = {cb};

  // Utilize pointer to pcb to be the id of LL item and add it to list
  item = ll_create_item(&udp_cb, sizeof(udp_cb), (uint32_t)pcb);
  if (pcb && item && ll_item_add(&CTX.udp_list, item) == BmOK) {
    // Bind UDP layer to created pcb
    udp_bind(pcb, IP_ANY_TYPE, port);
    udp_recv(pcb, udp_recv_cb, NULL);
  } else {
    if (pcb) {
      udp_remove(pcb);
    }
    if (item) {
      ll_delete_item(item);
    }
    pcb = NULL;
  }
  return (void *)pcb;
}

/*!
  @brief Create A New UDP Buffer

  @param size size of buffer to allocate in bytes

  @return pointer to created buffer
  @return NULL if unsuccessful
*/
void *bm_udp_new(uint32_t size) {
  return (void *)pbuf_alloc(PBUF_TRANSPORT, size, PBUF_RAM);
}

/*!
  @brief Get Pointer To Raw Payload Of Buffer

  @param buf buffer to obtain raw payload from

  @return raw payload
  @return NULL if invalid input parameters
*/
void *bm_udp_get_payload(void *buf) {
  void *ret = NULL;
  if (buf) {
    ret = ((struct pbuf *)buf)->payload;
  }
  return ret;
}

/*!
  @brief Update Reference Count Of Buffer

  @details This is utilized for counting the number of times that lwip must
           free this buffer when calling pbuf_free, other IP stacks might
           not utilize this

  @param buf buffer to update references on

  @return BmOK on success
  @return BmErr on failure
 */
BmErr bm_udp_reference_update(void *buf) {
  BmErr err = BmEINVAL;
  if (buf) {
    err = BmOK;
    pbuf_ref((struct pbuf *)buf);
  }
  return err;
}

/*!
  @brief Clean Up And Free UDP Buffer

  @param buf buffer to free memory
 */
void bm_udp_cleanup(void *buf) {
  if (buf) {
    pbuf_free((struct pbuf *)buf);
  }
}

/*!
 @brief Perform A UDP Transmission

 @param pcb protocol control block created with bm_udp_bind_port
 @param buf buffer to transmit
 @param addr ip address to transmit to
 @param port port to transmit to

 @return 
 */
BmErr bm_udp_tx_perform(void *pcb, void *buf, uint32_t size, const void *addr,
                        uint16_t port) {
  (void)size;
  BmErr err = BmEINVAL;
  if (buf && pcb && addr) {
    err = safe_udp_sendto_if((struct udp_pcb *)pcb, (struct pbuf *)buf,
                             (const ip_addr_t *)addr, port, CTX.netif) == ERR_OK
              ? BmOK
              : BmEBADMSG;
  }
  return err;
}
