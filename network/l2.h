#include "network_device.h"
#include "util.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef bm_l2_tx_task_priority
#define bm_l2_tx_task_priority 7
#endif

typedef void (*L2LinkChangeCb)(uint8_t port, bool state);

/*!
 @brief Handle Routing Of Link-Local Packets

 @details Per Bristlemouth specification in section 5.4.4.3, this is used to
          route Bristlemouth packets to other ports and determine if packets
          need to be handled beyond L2. 
          If egress_mask is 0 when this function returns, the packet is not
          forwarded.
          If the callback function returns true, the packet is sent up the
          Bristlemouth stack and handled by the specific application bound
          to the destination IP address

 @param ingress_port Port packet was received on
 @param egress_mask Mask of ports the packet should be forwarded to,
 @param src Source IP address of packet
 @param dest Destination IP address of packet 

 @return true if the packet should be submitted through the
              ip stack to the targeted application
         false if the packet should not be handled and
               just forwarded
 */
typedef bool (*L2LinkLocalRoutingCb)(uint8_t ingress_port,
                                     uint16_t *egress_mask, BmIpAddr *src,
                                     const BmIpAddr *dest);

BmErr bm_l2_handle_device_interrupt(void);
BmErr bm_l2_link_output(void *buf, uint32_t length);
void bm_l2_deinit(void);
BmErr bm_l2_init(NetworkDevice network_device);
BmErr bm_l2_register_link_change_callback(L2LinkChangeCb cb);
bool bm_l2_get_port_state(uint8_t port);
uint8_t bm_l2_get_port_count(void);
BmErr bm_l2_netif_set_power(bool on);
BmErr bm_l2_netif_enable_disable_port(uint8_t port_num, bool enable);
BmErr bm_l2_register_link_local_routing_callback(L2LinkLocalRoutingCb cb);

#ifdef __cplusplus
}
#endif
