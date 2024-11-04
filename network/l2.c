#include "l2.h"
#include "bm_config.h"
#include "bm_ip.h"
#include "bm_os.h"
#include "util.h"

#define ethernet_packet_size_byte 14
#define ipv6_version_traffic_class_flow_label_size_bytes 4
#define ipv6_payload_length_size_bytes 2
#define ipv6_next_header_size_bytes 1
#define ipv6_hop_limit_size_bytes 1
#define ipv6_source_address_size_bytes 16
#define ipv6_destination_address_size_bytes 16

#define ipv6_source_address_offset                                             \
  (ethernet_packet_size_byte +                                                 \
   ipv6_version_traffic_class_flow_label_size_bytes +                          \
   ipv6_payload_length_size_bytes + ipv6_next_header_size_bytes +              \
   ipv6_hop_limit_size_bytes)
#define ipv6_destination_address_offset                                        \
  (ipv6_source_address_offset + ipv6_source_address_size_bytes)

#define add_egress_port(addr, port)                                            \
  (addr[ipv6_source_address_offset + egress_port_idx] = port)
#define add_ingress_port(addr, port)                                           \
  (addr[ipv6_source_address_offset + ingress_port_idx] = port)
#define is_global_multicast(addr)                                              \
  (((uint8_t *)(addr))[ipv6_destination_address_offset] == 0xFFU &&            \
   ((uint8_t *)(addr))[ipv6_destination_address_offset + 1] == 0x03U)

#define evt_queue_len (32)

typedef enum {
  L2Tx,
  L2Rx,
  L2LinkUp,
  L2LinkDown,
  L2SetNetifUp,
  L2SetNetifDown,
} BmL2QueueType;

typedef struct {
  void *device_handle;
  uint8_t port_mask;
  void *buf;
  BmL2QueueType type;
  uint32_t length;
} L2QueueElement;

typedef struct {
  NetworkDevice network_device;
  uint8_t enabled_port_mask;
  BmQueue evt_queue;
  BmTaskHandle task_handle;
} BmL2Ctx;

static BmL2Ctx CTX = {0};

/*!
  @brief L2 TX Function

  @details Queues buffer to be sent over the network

  @param *buf buffer with frame/data to send out
  @param length size of buffer in bytes
  @param port_mask port(s) to transmit message over

  @return BmOK if successful
  @return BmErr if unsuccessful
*/
static BmErr bm_l2_tx(void *buf, uint32_t length, uint8_t port_mask) {
  BmErr err = BmOK;

  // device_handle not needed for tx
  // Don't send to ports that are offline
  L2QueueElement tx_evt = {NULL, (uint8_t)port_mask & CTX.enabled_port_mask,
                           buf, L2Tx, length};

  bm_l2_tx_prep(buf, length);
  if (bm_queue_send(CTX.evt_queue, &tx_evt, 10) != BmOK) {
    bm_l2_free(buf);
    err = BmENOMEM;
  }

  return err;
}

/*!
  @brief L2 RX Function - called by low level driver when new data is available

  @param device_handle device handle
  @param payload buffer with received data
  @param payload_len buffer length in bytes
  @param port_mask which port was this received over

  @return BmOK if successful
  @return BmErr if unsuccsessful
*/
static BmErr bm_l2_rx(void *device_handle, uint8_t *payload,
                      uint16_t payload_len, uint8_t port_mask) {
  BmErr err = BmEINVAL;
  L2QueueElement rx_evt = {device_handle, port_mask, NULL, L2Rx, payload_len};

  if (device_handle && payload) {
    err = BmOK;
    rx_evt.buf = bm_l2_new(payload_len);
    if (rx_evt.buf == NULL) {
      bm_debug("No mem for buf in RX pathway\n");
      err = BmENOMEM;
    } else {
      memcpy(bm_l2_get_payload(rx_evt.buf), payload, payload_len);

      if (bm_queue_send(CTX.evt_queue, (void *)&rx_evt, 0) != BmOK) {
        bm_l2_free(rx_evt.buf);
        err = BmENOMEM;
      }
    }
  }

  return err;
}

/*!
  @brief Process TX Event

  @details Receive message from L2 queue and send to the network device,
           the specific ports to use are stored in the tx_evt data structure

  @param *tx_evt tx event with buffer, port, and other information
*/
static void bm_l2_process_tx_evt(L2QueueElement *tx_evt) {
  if (tx_evt) {
    uint8_t *payload = (uint8_t *)bm_l2_get_payload(tx_evt->buf);
    BmErr err = CTX.network_device.trait->send(
        CTX.network_device.self, payload, tx_evt->length, tx_evt->port_mask);
    if (err != BmOK) {
      bm_debug("Failed to send TX buffer to network\n");
    }
    bm_l2_free(tx_evt->buf);
  }
}

/*!
  @brief Process RX event

  @details Receive message from L2 queue and:
             1. re-transmit over other port if the message is multicast
             2. send up to lwip for processing via net_if->input()

  @param *rx_evt - rx event with buffer, port, and other information
*/
static void bm_l2_process_rx_evt(L2QueueElement *rx_evt) {
  if (rx_evt) {
    uint8_t *payload = (uint8_t *)bm_l2_get_payload(rx_evt->buf);

    // We need to code the RX Port into the IPV6 address passed up the stack
    add_ingress_port(payload, rx_evt->port_mask);

    if (is_global_multicast(payload)) {
      uint8_t new_port_mask = ADIN2111_PORT_MASK & ~(rx_evt->port_mask);
      bm_l2_tx(rx_evt->buf, rx_evt->length, new_port_mask);
    }

    /* TODO: This is the place where routing and filtering functions would happen, to prevent passing the
       packet to net_if->input() if unnecessary, as well as forwarding routed multicast data to interested
       neighbors and user devices. */

    // Submit packet to ip stack.
    // Upper level RX Callback is responsible for freeing the packet
    bm_l2_submit(rx_evt->buf, rx_evt->length);
  }
}

/*!
  @brief L2 thread which handles both tx and rx events

  @param *parameters - unused
*/
static void bm_l2_thread(void *parameters) {
  (void)parameters;

  while (true) {
    L2QueueElement event;
    if (bm_queue_receive(CTX.evt_queue, &event, UINT32_MAX) == BmOK) {
      switch (event.type) {
      case L2Tx: {
        bm_l2_process_tx_evt(&event);
        break;
      }
      case L2Rx: {
        bm_l2_process_rx_evt(&event);
        break;
      }
      default: {
      }
      }
    }
  }
}

/*!
  @brief Deinitialize L2 Layer

  @details Frees and deinitializes all things L2
 */
void bm_l2_deinit(void) {
  if (CTX.evt_queue) {
    bm_queue_delete(CTX.evt_queue);
  }
  if (CTX.task_handle) {
    bm_task_delete(CTX.task_handle);
  }
  memset(&CTX, 0, sizeof(BmL2Ctx));
}

/*!
  @brief Initialize L2 layer
  @details bm_l2_deinit must be called before this is called again to free resources
  @param netif The already-initialized network interface
  @return BmOK if successful, an error otherwise
 */
BmErr bm_l2_init(NetworkDevice network_device) {
  BmErr err = BmEINVAL;
  CTX.network_device = network_device;
  CTX.evt_queue = bm_queue_create(evt_queue_len, sizeof(L2QueueElement));
  if (CTX.evt_queue) {
    err = bm_task_create(bm_l2_thread, "L2 TX Thread", 2048, NULL,
                         bm_l2_tx_task_priority, CTX.task_handle);
  } else {
    err = BmENOMEM;
  }
  return err;
}

/*!
  @brief bm_l2_tx wrapper for network stack

  @param *buf buffer of data to transmit
  @param length buffer size in bytes

  @return BmOK if successful
  @return BMErr if unsuccessful
*/
BmErr bm_l2_link_output(void *buf, uint32_t length) {
  // by default, send to all ports
  uint8_t port_mask = ADIN2111_PORT_MASK;

  // if the application set an egress port, send only to that port
  static const size_t bcmp_egress_port_offset_in_dest_addr = 13;
  const size_t egress_idx =
      ipv6_destination_address_offset + bcmp_egress_port_offset_in_dest_addr;
  uint8_t *eth_frame = (uint8_t *)bm_l2_get_payload(buf);
  uint8_t egress_port = eth_frame[egress_idx];

  if (egress_port > 0 && egress_port <= ADIN2111_PORT_NUM) {
    port_mask = 1 << (egress_port - 1);
  }

  // clear the egress port set by the application
  eth_frame[egress_idx] = 0;

  return bm_l2_tx(buf, length, port_mask);
}

/*!
  @brief Obtains The State Of The Provided Port

  @param port port to obtain the state of

  @return true if port is online
  @retunr false if port is offline
 */
bool bm_l2_get_port_state(uint8_t port) {
  return (bool)(CTX.enabled_port_mask & (1 << port));
}

/*!
  @brief Public api to set the network interface on/off
  @param on - true to turn the interface on, false to turn the interface off.
*/
BmErr bm_l2_netif_set_power(bool on) {
  BmErr err = BmOK;
  NetworkDevice device = CTX.network_device;
  if (on) {
    bm_err_check(err, device.trait->enable(device.self));
    bm_err_check(err, bm_l2_set_netif(true));
  } else {
    bm_err_check(err, bm_l2_set_netif(false));
    bm_err_check(err, device.trait->disable(device.self));
  }
  return err;
}
