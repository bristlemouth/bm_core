#include "l2.h"
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

typedef struct {
  BmNetDevCfg cfg;
  uint8_t start_port_idx;
  uint8_t num_ports_mask;
} BmNetdevCtx;

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
  BmNetdevCtx *devices;
  BmL2ModuleLinkChangeCb link_change_cb;
  uint8_t available_ports_mask;
  uint8_t available_port_mask_idx;
  uint8_t enabled_port_mask;
  uint8_t num_devices;
  BmQueue evt_queue;
} BmL2Ctx;

static BmL2Ctx CTX;

/*!
  @brief Obtain The Index Of A Device By Its Handle

  @param device_handle handle of device to get index for

  @return index of device if found
  @return -1 if not found
 */
static int32_t bm_l2_get_device_index(const void *device_handle) {
  int32_t rval = -1;

  for (int32_t idx = 0; idx < CTX.num_devices; idx++) {
    if (device_handle == CTX.devices[idx].cfg.device_handle) {
      rval = idx;
    }
  }

  return rval;
}

/*!
  @brief Turn Port Count To Bit Mask

  @param count number of ports

  @return bitmask of number of ports
 */
static uint8_t port_count_to_bit_mask(uint8_t count) {
  uint8_t ret = 0;
  for (uint8_t i = 0; i < count; i++) {
    ret |= 1 << i;
  }
  return ret;
}

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
      printf("No mem for buf in RX pathway\n");
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

  @details Receive message from L2 queue and send over all
           network interfaces (if there are multiple), the specific port
           to use is stored in the tx_event data structure

  @param *tx_evt tx event with buffer, port, and other information
*/
static void bm_l2_process_tx_evt(L2QueueElement *tx_evt) {
  uint8_t mask_idx = 0;

  if (tx_evt) {
    for (uint32_t idx = 0; idx < CTX.num_devices; idx++) {
      if (CTX.devices[idx].cfg.tx_cb) {
        BmErr retv = CTX.devices[idx].cfg.tx_cb(
            CTX.devices[idx].cfg.device_handle,
            (uint8_t *)bm_l2_get_payload(tx_evt->buf), tx_evt->length,
            (tx_evt->port_mask >> mask_idx) & CTX.devices[idx].num_ports_mask,
            CTX.devices[idx].start_port_idx);
        mask_idx += CTX.devices[idx].cfg.num_ports;
        if (retv != BmOK) {
          printf("Failed to submit TX buffer\n");
        }
      }
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
    uint8_t rx_port_mask = 0;
    uint8_t device_idx = bm_l2_get_device_index(rx_evt->device_handle);
    if (CTX.devices[device_idx].cfg.device_handle) {
      rx_port_mask =
          ((rx_evt->port_mask & CTX.devices[device_idx].num_ports_mask)
           << CTX.devices[device_idx].start_port_idx);
      // We need to code the RX Port into the IPV6 address passed up the stack
      add_ingress_port(((uint8_t *)bm_l2_get_payload(rx_evt->buf)),
                       rx_port_mask);

      if (is_global_multicast(bm_l2_get_payload(rx_evt->buf))) {
        uint8_t new_port_mask = CTX.available_ports_mask & ~(rx_port_mask);
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
}

/*!
  @brief Link change callback function called from eth driver
         whenever the state of a link changes

  @param *device_handle eth driver handle
  @param port (device specific) port that changed
  @param state up/down
*/
static void link_change_cb(void *device_handle, uint8_t port, bool state) {

  L2QueueElement link_change_evt = {device_handle, port, NULL, L2LinkDown, 0};
  if (state) {
    link_change_evt.type = L2LinkUp;
  }

  if (device_handle) {
    // Schedule link change event
    bm_queue_send(CTX.evt_queue, &link_change_evt, 10);
  }
}

/*!
  @brief Handle link change event

  @param *device_handle eth driver handle
  @param port (device specific) port that changed
  @param state up/down

  @return none
*/
static void handle_link_change(const void *device_handle, uint8_t port,
                               bool state) {
  if (device_handle) {
    for (uint8_t device = 0; device < CTX.num_devices; device++) {
      if (device_handle == CTX.devices[device].cfg.device_handle) {
        // Get the overall port number
        uint8_t port_idx = CTX.devices[device].start_port_idx + port;
        // Keep track of what ports are enabled/disabled so we don't try and send messages
        // out through them
        uint8_t port_mask = 1 << (port_idx);
        if (state) {
          CTX.enabled_port_mask |= port_mask;
        } else {
          CTX.enabled_port_mask &= ~port_mask;
        }

        if (CTX.link_change_cb) {
          CTX.link_change_cb(port_idx, state);
        } else {
          printf("port%u %s\n", port_idx, state ? "up" : "down");
        }
      }
    }
  }
}

/*!
  @brief Handle netif set event

  @param device_handle eth driver handle
  @param on true to turn the interface on, false to turn the interface off.
*/
static void bm_l2_process_netif_evt(const void *device_handle, bool on) {
  for (uint8_t device = 0; device < CTX.num_devices; device++) {
    if (device_handle == CTX.devices[device].cfg.device_handle) {
      if (CTX.devices[device].cfg.power_cb) {
        if (CTX.devices[device].cfg.power_cb(
                (void *)device_handle, on, CTX.devices[device].cfg.port_mask) ==
            0) {
          printf("Powered device %d : %s\n", device, (on) ? "on" : "off");
        } else {
          printf("Failed to power device %d : %s\n", device,
                 (on) ? "on" : "off");
        }
      }
    }
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
      case L2LinkUp: {
        handle_link_change(event.device_handle, event.port_mask, true);
        break;
      }
      case L2LinkDown: {
        handle_link_change(event.device_handle, event.port_mask, false);
        break;
      }
      case L2SetNetifUp: {
        bm_l2_process_netif_evt(event.device_handle, true);
        bm_l2_set_netif(true);
        break;
      }
      case L2SetNetifDown: {
        bm_l2_set_netif(false);
        bm_l2_process_netif_evt(event.device_handle, false);
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
  bm_free(CTX.devices);
  memset(&CTX, 0, sizeof(BmL2Ctx));
}

/*!
  @brief Initialize L2 layer

  @details bm_l2_deinit must be called before this is called again to free
           resources

  @param cb link change callback that will be called when the ethernet driver
            changes the link status
  @param cfg pointer to array of network device configurations
  @param count number of network device configurations

  @return BmOK if successful
  @return BmErr if unsuccessful
 */
BmErr bm_l2_init(BmL2ModuleLinkChangeCb cb, const BmNetDevCfg *cfg,
                 uint8_t count) {
  BmErr err = BmENODEV;
  uint32_t size_devices = sizeof(BmNetdevCtx) * count;

  CTX.link_change_cb = cb;

  /* Reset context variables */
  CTX.available_ports_mask = 0;
  CTX.available_port_mask_idx = 0;

  if (cfg && size_devices) {
    CTX.num_devices = count;
    CTX.devices = (BmNetdevCtx *)bm_malloc(size_devices);
    memset(CTX.devices, 0, size_devices);
    for (uint32_t idx = 0; idx < count; idx++) {
      // Only assign variables if there is a device handle and the device
      // can be initialized
      if (cfg[idx].device_handle && cfg[idx].init_cb &&
          cfg[idx].init_cb(cfg[idx].device_handle, bm_l2_rx, link_change_cb,
                           cfg[idx].port_mask) == BmOK) {
        CTX.devices[idx].cfg = cfg[idx];
        CTX.devices[idx].start_port_idx = CTX.available_port_mask_idx;
        CTX.devices[idx].num_ports_mask =
            port_count_to_bit_mask(CTX.devices[idx].cfg.num_ports);
        CTX.available_ports_mask |=
            (CTX.devices[idx].num_ports_mask << CTX.available_port_mask_idx);
        CTX.available_port_mask_idx += CTX.devices[idx].cfg.num_ports;
      } else {
        printf("Failed to init module at index %d\n", idx);
      }
    }
    CTX.evt_queue = bm_queue_create(evt_queue_len, sizeof(L2QueueElement));
    if (CTX.evt_queue) {
      err = bm_task_create(bm_l2_thread, "L2 TX Thread", 2048, NULL,
                           bm_l2_tx_task_priority, NULL);
    } else {
      err = BmENOMEM;
    }
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
  uint8_t port_mask = CTX.available_ports_mask;

  // if the application set an egress port, send only to that port
  static const size_t bcmp_egress_port_offset_in_dest_addr = 13;
  const size_t egress_idx =
      ipv6_destination_address_offset + bcmp_egress_port_offset_in_dest_addr;
  uint8_t *eth_frame = (uint8_t *)bm_l2_get_payload(buf);
  uint8_t egress_port = eth_frame[egress_idx];

  if (egress_port > 0 && egress_port <= CTX.available_port_mask_idx) {
    port_mask = 1 << (egress_port - 1);
  }

  // clear the egress port set by the application
  eth_frame[egress_idx] = 0;

  return bm_l2_tx(buf, length, port_mask);
}

/*!
  @brief Get the total number of ports

  @return number of ports
*/
uint8_t bm_l2_get_num_ports(void) { return CTX.available_port_mask_idx; }

/*!
 @brief Get The Total Number Of Devices

 @return number of devices
 */
uint8_t bm_l2_get_num_devices(void) { return CTX.num_devices; }

/*!
  @brief Get the raw device handle for a specific port

  @param dev_idx port index to get the handle from
  @param *device_handle pointer to variable to store device handle in
  @param *start_port_idx pointer to variable to store the start port for this device

  @return true if successful
  @return false if unsuccessful
*/
bool bm_l2_get_device_handle(uint8_t dev_idx, void **device_handle,
                             uint32_t *start_port_idx) {
  bool rval = false;

  if (device_handle && start_port_idx && dev_idx < CTX.num_devices) {
    *device_handle = CTX.devices[dev_idx].cfg.device_handle;
    *start_port_idx = CTX.devices[dev_idx].start_port_idx;
    rval = true;
  }

  return rval;
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
  @brief Public api to set a particular network device on/off

  @param device_handle eth driver handle
  @param on - true to turn the interface on, false to turn the interface off.
*/
BmErr bm_l2_netif_set_power(void *device_handle, bool on) {
  BmErr err = BmEINVAL;
  L2QueueElement pwr_evt = {device_handle, 0, NULL, L2SetNetifDown, 0};

  if (device_handle) {
    if (on) {
      pwr_evt.type = L2SetNetifUp;
    }

    // Schedule netif pwr event
    err = bm_queue_send(CTX.evt_queue, &pwr_evt, 10);
  }

  return err;
}
