#include "bm_adin2111.h"
#include "aligned_malloc.h"
#include "bm_config.h"
#include "bm_os.h"
#include <stdbool.h>
#include <string.h>

// Extra 4 bytes for FCS and 2 bytes for the frame header
#define MAX_FRAME_BUF_SIZE (MAX_FRAME_SIZE + 4 + 2)
#define DMA_ALIGN_SIZE (4)

struct LinkChange {
  void *device_handle;
  adin2111_Port_e port_index;
};

// For now, there's only ever one ADIN2111.
// When supporting multiple in the future,
// we can allocate this dynamically.
static NetworkDevice NETWORK_DEVICE;
static adin2111_DeviceStruct_t DEVICE_STRUCT;
#ifdef ENABLE_TESTING
static uint8_t DEVICE_MEMORY[ADIN2111_DEVICE_SIZE + 80];
#else
static uint8_t DEVICE_MEMORY[ADIN2111_DEVICE_SIZE];
#endif
static adin2111_DriverConfig_t DRIVER_CONFIG = {
    .pDevMem = (void *)DEVICE_MEMORY,
    .devMemSize = sizeof(DEVICE_MEMORY),
    .fcsCheckEn = false,
    .tsTimerPin = ADIN2111_TS_TIMER_MUX_NA,
    .tsCaptPin = ADIN2111_TS_CAPT_MUX_NA,
};
static adi_eth_BufDesc_t RX_BUFFERS[RX_QUEUE_NUM_ENTRIES];
static HAL_Callback_t ADIN2111_MAC_INT_CALLBACK = NULL;
static void *ADIN2111_MAC_INT_CALLBACK_PARAM = NULL;
static struct LinkChange LINK_CHANGE = {NULL, ADIN2111_PORT_1};

/**************** Private Helper Functions ****************/
/*!
  @brief Register interrupt pin callback for the ADIN2111 network device

  @details Required by the Analog Devices adi_hal.h to be implementeed by the application.
           We save the pointer here in our driver wrapper to simplify Bristlemouth integration.

  @param intCallback callback function to be used on the actual pin interrupt
  @param hDevice network device pointer

  @return ADI_ETH_SUCCESS
 */
uint32_t HAL_RegisterCallback(HAL_Callback_t const *intCallback,
                              void *hDevice) {
  // Analog Devices code has a bug at adi_mac.c:633 where they
  // cast a function pointer to a function pointer pointer incorrectly.
  ADIN2111_MAC_INT_CALLBACK = (const HAL_Callback_t)intCallback;
  ADIN2111_MAC_INT_CALLBACK_PARAM = hDevice;
  return ADI_ETH_SUCCESS;
}

/*!
  @brief Trigger autonegotiation on the ADIN device
  
  @details Will retrigger autonegotiation on the ADIN device. The status of the
           ADIN's autonegotiation should be polled before calling this API to
           determine if this is an appropriate time to renegotiate. This
           function is an addition to the ADIN driver (adin2111.c) and
           necessary for Bristlemouth.

  @param hDevice Device driver handle
  @param port Port number
  
  @return ADI_ETH_SUCCESS on pass, other on failure
 */
static adi_eth_Result_e adin2111_Renegotiate(adin2111_DeviceHandle_t hDevice,
                                             adin2111_Port_e port) {
  adi_eth_Result_e result = ADI_ETH_SUCCESS;

  if ((port != ADIN2111_PORT_1) && (port != ADIN2111_PORT_2)) {
    result = ADI_ETH_INVALID_PORT;
  } else {
    result = phyDriverEntry.Renegotiate(hDevice->pPhyDevice[port]);
  }

  return result;
}

/*!
  @brief Get the status of autonegotiation
 
  @details Determines the status of the current autonegotiation that has
           occured. Will return ADI_ETH_SUCCESS if status was able to be
           obtained from the ADIN device. This function is an addition to
           the ADIN driver (adin2111.c) and necessary for Bristlemouth.

  @param hDevice Device driver handle
  @param port Port number
  @param status Status of autonegotiation
 
  @return ADI_ETH_SUCCESS on pass, other on failure
 */
static adi_eth_Result_e
adin2111_AutoNegotiateStatus(adin2111_DeviceHandle_t hDevice,
                             adin2111_Port_e port, adi_phy_AnStatus_t *status) {
  adi_eth_Result_e result = ADI_ETH_SUCCESS;

  if ((port != ADIN2111_PORT_1) && (port != ADIN2111_PORT_2)) {
    result = ADI_ETH_INVALID_PORT;
  } else {
    result = phyDriverEntry.GetAnStatus(hDevice->pPhyDevice[port], status);
  }

  return result;
}

/*!
 @brief ADIN2111 network device link change callback

 @details Called by the driver when the link status changes,
          if the user has registered a callback, setup variables
          necessary for when there is an interrupt on the device

 @param device_handle network device pointer
 @param event unused
 @param status_registers_param registers with information on which port has a
                               link change event
 */
static void link_change_callback_(void *device_handle, uint32_t event,
                                  void *status_registers_param) {
  (void)event;

  if (NETWORK_DEVICE.callbacks->link_change) {
    const adi_mac_StatusRegisters_t *status_registers =
        (adi_mac_StatusRegisters_t *)status_registers_param;

    if (status_registers->p1StatusMasked == ADI_PHY_EVT_LINK_STAT_CHANGE) {
      LINK_CHANGE.port_index = ADIN2111_PORT_1;
      LINK_CHANGE.device_handle = device_handle;
    } else if (status_registers->p2StatusMasked ==
               ADI_PHY_EVT_LINK_STAT_CHANGE) {
      LINK_CHANGE.port_index = ADIN2111_PORT_2;
      LINK_CHANGE.device_handle = device_handle;
    }
  }
}

/*!
 @brief Obtain the ADIN2111 driver's transmission port

 @details Convert the port number to the driver's port enumeration

 @param port_num Port number, 1/2/x, x will flood all ports

 @return Port to transmit data on
 */
static inline adin2111_TxPort_e driver_tx_port(uint8_t port_num) {
  switch (port_num) {
  case 1:
    return ADIN2111_TX_PORT_1;
  case 2:
    return ADIN2111_TX_PORT_2;
  default:
    return ADIN2111_TX_PORT_FLOOD;
  }
}

/*!
 @brief Obtain the ADIN2111 driver's generic port

 @details Convert the port number to the driver's port enumeration

 @param port_num Port number, 1 or 2

 @return Port for ADIN2111 driver API functions
 */
static inline adin2111_Port_e driver_port(uint8_t port_num) {
  switch (port_num) {
  case 1:
    return ADIN2111_PORT_1;
  case 2:
    return ADIN2111_PORT_2;
  default:
    return ADIN2111_PORT_NUM;
  }
}

/*!
  @brief Enable single network device port

  @param port_num port number to enable, 1 or 2

  @return BmOK on success
  @return BmErr on failure
 */
inline static BmErr adin2111_netdevice_enable_port(uint8_t port_num) {
  BmErr err = BmEINVAL;
  adin2111_Port_e enable_port = driver_port(port_num);

  switch (enable_port) {
  case ADIN2111_PORT_1:
  case ADIN2111_PORT_2:
    if (adin2111_EnablePort(&DEVICE_STRUCT, enable_port) != ADI_ETH_SUCCESS) {
      err = BmENODEV;
    } else {
      err = BmOK;
    }
    break;
  default:
    break;
  }

  return err;
}

/*!
  @brief Enable single network device port

  @details Trait wrapper function for adin2111_netdevice_enable_port

  @param self unused
  @param port_num port number to enable, 1 or 2

  @return BmOK on success
  @return BmErr on failure
 */
inline static BmErr adin2111_netdevice_enable_port_(void *self,
                                                    uint8_t port_num) {
  (void)self;
  return adin2111_netdevice_enable_port(port_num);
}

/*!
  @brief Disable single network device port

  @details The ADIN2111 can only disable port 2 individually, if port 1 is shut
           down, so is port 2

  @param port_num port number to disable, 1 or 2

  @return BmOK on success
  @return BmErr on failure
 */
inline static BmErr adin2111_netdevice_disable_port(uint8_t port_num) {
  BmErr err = BmEINVAL;
  adi_eth_Result_e result = ADI_ETH_SUCCESS;
  adin2111_Port_e disable_port = driver_port(port_num);

  switch (disable_port) {
  // WARNING: If port 1 is disabled, port 2 will also be disabled as per page
  // 25 of the Rev B ADIN2111 datasheet:
  // https://www.analog.com/media/en/technical-documentation/data-sheets/adin2111.pdf
  case ADIN2111_PORT_1:
  case ADIN2111_PORT_2:
    result = adin2111_DisablePort(&DEVICE_STRUCT, disable_port);
    if (result != ADI_ETH_SUCCESS) {
      err = BmENODEV;
    } else {
      err = BmOK;
    }
    break;
  default:
    break;
  }

  return err;
}

/*!
  @brief Disable single network device port

  @details Trait wrapper function for adin2111_netdevice_disable_port

  @param self unused
  @param port_num port number to disable, 1 or 2

  @return BmOK on success
  @return BmErr on failure
 */
static BmErr adin2111_netdevice_disable_port_(void *self, uint8_t port_num) {
  (void)self;
  return adin2111_netdevice_disable_port(port_num);
}

/*!
  @brief Enable the network device

  @details This turns on the power to the ADIN2111, initializes is,
           registers the proper callbacks and setups buffers for the ADIN2111
           driver

  @return BmOK on success
  @return BmErr on failure
 */
static BmErr adin2111_netdevice_enable(void) {
  BmErr err = BmOK;

  if (NETWORK_DEVICE.callbacks->power) {
    NETWORK_DEVICE.callbacks->power(true);
  }

  adi_eth_Result_e result = adin2111_Init(&DEVICE_STRUCT, &DRIVER_CONFIG);
  if (result != ADI_ETH_SUCCESS) {
    err = BmENODEV;
    goto end;
  }

  result = adin2111_RegisterCallback(&DEVICE_STRUCT, link_change_callback_,
                                     ADI_MAC_EVT_LINK_CHANGE);
  if (result != ADI_ETH_SUCCESS) {
    err = BmENODEV;
    goto end;
  }

  for (int i = 0; i < RX_QUEUE_NUM_ENTRIES; i++) {
    // Buffers must already have been allocated and initialized in adin2111_init
    adi_eth_BufDesc_t *buffer_description = &RX_BUFFERS[i];
    if (!buffer_description || !buffer_description->pBuf) {
      err = BmENODEV;
      goto end;
    }
    result = adin2111_SubmitRxBuffer(&DEVICE_STRUCT, buffer_description);
    if (result != ADI_ETH_SUCCESS) {
      err = BmENODEV;
      goto end;
    }
  }

  result = adin2111_SyncConfig(&DEVICE_STRUCT);
  if (result != ADI_ETH_SUCCESS) {
    err = BmENODEV;
    goto end;
  }

  for (int i = 1; i <= ADIN2111_PORT_NUM; i++) {
    err = adin2111_netdevice_enable_port(i);
    if (err != BmOK) {
      break;
    }
  }

end:
  if (err != BmOK && NETWORK_DEVICE.callbacks->power) {
    NETWORK_DEVICE.callbacks->power(false);
  }

  return err;
}

/*!
  @brief Enable the network device

  @details Trait wrapper function to convert self from void* to Adin2111*

  @param self unused

  @return BmOK on success
  @return BmErr on failure
 */
inline static BmErr adin2111_netdevice_enable_(void *self) {
  (void)self;
  return adin2111_netdevice_enable();
}

/*!
  @brief Disable the network device

  @details Shut down and disable the ADIN2111 hardware.

  @return BmOK on success
  @return BmErr on failure
 */
static BmErr adin2111_netdevice_disable(void) {
  BmErr err = BmEINVAL;

  for (int i = 1; i <= ADIN2111_PORT_NUM; i++) {
    err = adin2111_netdevice_disable_port(i);
    if (err != BmOK) {
      break;
    }
  }

  if (err == BmOK && NETWORK_DEVICE.callbacks->power) {
    NETWORK_DEVICE.callbacks->power(false);
  }

  return err;
}

/*!
  @brief Disable the network device

  @details Trait wrapper function to convert self from void* to Adin2111*

  @param self unused

  @return BmOK on success
  @return BmErr on failure
 */
inline static BmErr adin2111_netdevice_disable_(void *self) {
  (void)self;
  return adin2111_netdevice_disable();
}

/*!
  @brief Renegotiate leader/follower resolution mechanic

  @details Function to determine if the ADIN2111 should re-activate
           autonegotiation for its leader/follower mechanic, this will see if
           autonegotiation has been completed, and if so, determine if a fault
           occurred, if a fault has occurred, re-trigger autonegotiation

  @param port_num port number to determine if port autonegotiation shall retry
  @param renegotiated if the autonegotiation was retriggered

  @return BmOK on success
  @return BmErr on failure
 */
inline static BmErr adin2111_netdevice_renegotiate(uint8_t port_num,
                                                   bool *renegotiated) {
  BmErr err = BmEINVAL;
  adi_eth_Result_e result = ADI_ETH_SUCCESS;
  adi_phy_AnStatus_t status = {};
  adin2111_Port_e port = driver_port(port_num);

  switch (port) {
  case ADIN2111_PORT_1:
  case ADIN2111_PORT_2:
    err = BmOK;
    *renegotiated = false;
    result = adin2111_AutoNegotiateStatus(&DEVICE_STRUCT, port, &status);
    if (result == ADI_ETH_SUCCESS &&
        (status.anComplete &&
         status.anMsResolution != ADI_PHY_AN_MS_RESOLUTION_SLAVE &&
         status.anMsResolution != ADI_PHY_AN_MS_RESOLUTION_MASTER)) {
      result = adin2111_Renegotiate(&DEVICE_STRUCT, port);
      *renegotiated = true;
    }

    if (result != ADI_ETH_SUCCESS) {
      err = BmENODEV;
    }
    break;
  default:
    break;
  }

  return err;
}

/*!
  @brief Renegotiate leader/follower resolution mechanic

  @details Trait wrapper function to determine if the ADIN2111 should
           renegotiate its leader/follower mechanic to another network device

  @param self unused
  @param port_num port number to renegotiate on
  @param renegotiated if the negotiation was triggered

  @return BmOK on success
  @return BmErr on failure
 */
static BmErr adin2111_netdevice_renegotiate_(void *self, uint8_t port_num,
                                             bool *renegotiated) {
  (void)self;
  return adin2111_netdevice_renegotiate(port_num, renegotiated);
}

/*!
  @brief After a TX buffer is sent, it gets freed here

  @param buffer_description ADIN2111 driver buffer description of what was just
                            transmitted
 */
static void free_tx_buffer(adi_eth_BufDesc_t *buffer_description) {
  if (buffer_description) {
    if (buffer_description->pBuf) {
      aligned_free(buffer_description->pBuf);
    }
    bm_free(buffer_description);
  }
}

/*!
  @brief This is the callback that the driver calls after a TX buffer is sent

  @param device_param unused
  @param event unused
  @param buffer_description ADIN2111 driver buffer description of what was just
                            transmitted
 */
static void tx_complete(void *device_param, uint32_t event,
                        void *buffer_description) {
  (void)device_param;
  (void)event;

  free_tx_buffer(buffer_description);
}

/*!
  @brief Transmit data to network

  @details Allocate buffers for sending, copy the given data, and submit to the driver

  @param data pointer to data to send over the network
  @param length data length
  @param port port to transmit data onto

  @return BmOk on success
  @return BmErr on failure
 */
static BmErr adin2111_netdevice_send(uint8_t *data, size_t length,
                                     uint8_t port) {
  BmErr err = BmOK;

  adi_eth_BufDesc_t *buffer_description = bm_malloc(sizeof(adi_eth_BufDesc_t));
  if (!buffer_description) {
    err = BmENOMEM;
    goto end;
  }
  memset(buffer_description, 0, sizeof(adi_eth_BufDesc_t));
  buffer_description->pBuf = aligned_malloc(DMA_ALIGN_SIZE, length);
  if (!buffer_description->pBuf) {
    bm_free(buffer_description);
    err = BmENOMEM;
    goto end;
  }
  memcpy(buffer_description->pBuf, data, length);
  buffer_description->trxSize = length;
  buffer_description->cbFunc = tx_complete;
  adin2111_TxPort_e tx_port = driver_tx_port(port);
  adi_eth_Result_e result =
      adin2111_SubmitTxBuffer(&DEVICE_STRUCT, tx_port, buffer_description);
  if (result != ADI_ETH_SUCCESS) {
    free_tx_buffer(buffer_description);
    goto end;
  }

  if (NETWORK_DEVICE.callbacks->debug_packet_dump) {
    NETWORK_DEVICE.callbacks->debug_packet_dump(data, length);
  }

end:
  return err;
}

/*!
  @brief Transmit data to network

  @details Trait wrapper function to convert self from void* to Adin2111*

  @param self unused
  @param data pointer to data to send over the network
  @param length data length
  @param port port to transmit data onto

  @return BmOk on success
  @return BmErr on failure
 */
static inline BmErr adin2111_netdevice_send_(void *self, uint8_t *data,
                                             size_t length, uint8_t port) {
  (void)self;
  return adin2111_netdevice_send(data, length, port);
}

/*!
  @brief ADIN2111 driver receive callback

  @details Called by the driver on received data,
           if the user has registered a callback, call it

  @param device unused
  @param event unused
  @param buffer_description_param
 */
static void receive_callback(void *device, uint32_t event,
                             void *buffer_description_param) {
  (void)device;
  (void)event;
  adi_eth_BufDesc_t *buffer_description =
      (adi_eth_BufDesc_t *)buffer_description_param;

  if (NETWORK_DEVICE.callbacks->receive) {
    // Driver gives us zero or one. Bristlemouth spec ingress port is 1-15.
    uint8_t port_num = buffer_description->port + 1;
    NETWORK_DEVICE.callbacks->receive(port_num, buffer_description->pBuf,
                                      buffer_description->trxSize);
  }

  if (NETWORK_DEVICE.callbacks->debug_packet_dump) {
    NETWORK_DEVICE.callbacks->debug_packet_dump(buffer_description->pBuf,
                                                buffer_description->trxSize);
  }

  // Re-submit buffer into ADIN's RX queue
  adi_eth_Result_e result =
      adin2111_SubmitRxBuffer(&DEVICE_STRUCT, buffer_description);
  if (result != ADI_ETH_SUCCESS) {
    bm_debug("Unable to re-submit RX Buffer\n");
  }
}

/*!
  @brief Obtain number of ports on network device

  @details Trait function to obtain the number of ports on the ADIN2111

  @return ADIN2111_PORT_NUM
 */
static inline uint8_t adin2111_num_ports(void) { return ADIN2111_PORT_NUM; }

/*!
  @brief Obtain statistics from the network device

  @details Get ADIN2111 specific statistics for a port

  @param port_index port index to obtain stats on, 0 or 1
  @param stats pointer to the stats to be obtained

  @return BmOk on success
  @return BmErr on failure
 */
static BmErr adin2111_port_stats(uint8_t port_index, Adin2111PortStats *stats) {
  BmErr err = BmOK;

  if (stats == NULL) {
    err = BmEINVAL;
    goto end;
  }

  memset(stats, 0, sizeof(Adin2111PortStats));

  adi_eth_Result_e result = adin2111_GetMseLinkQuality(
      &DEVICE_STRUCT, port_index, &stats->mse_link_quality);
  if (result != ADI_ETH_SUCCESS) {
    err = BmENODEV;
    goto end;
  }

  result = adin2111_FrameChkReadRxErrCnt(&DEVICE_STRUCT, port_index,
                                         &stats->frame_check_rx_error_count);
  if (result != ADI_ETH_SUCCESS) {
    err = BmENODEV;
    goto end;
  }

  result = adin2111_FrameChkReadErrorCnt(&DEVICE_STRUCT, port_index,
                                         &stats->frame_check_error_counters);
  if (result != ADI_ETH_SUCCESS) {
    err = BmENODEV;
  }

end:
  return err;
}

/*!
  @brief Obtain statistics from the network device

  @details Trait wrapper function to convert void* to device-specific types

  @param self unused
  @param port_index port index to obtain stats on, 0 or 1
  @param stats pointer to the stats to be obtained

  @return BmOk on success
  @return BmErr on failure
 */
static inline BmErr adin2111_port_stats_(void *self, uint8_t port_index,
                                         void *stats) {
  (void)self;
  return adin2111_port_stats(port_index, stats);
}

/*!
  @brief Handle network device interrupt

  @details L2 calls this trait function from a thread
           after getting out of the external pin interrupt context.

  @param self unused

  @return BmOK on success
  @return BmErr on failure
 */
static BmErr adin2111_handle_interrupt(void *self) {
  (void)self;
  BmErr err = BmENODEV;
  if (ADIN2111_MAC_INT_CALLBACK) {
    ADIN2111_MAC_INT_CALLBACK(ADIN2111_MAC_INT_CALLBACK_PARAM, 0, NULL);
    if (LINK_CHANGE.device_handle) {
      adi_eth_LinkStatus_e status;
      adi_eth_Result_e result = adin2111_GetLinkStatus(
          LINK_CHANGE.device_handle, LINK_CHANGE.port_index, &status);
      if (result == ADI_ETH_SUCCESS) {
        NETWORK_DEVICE.callbacks->link_change((uint8_t)LINK_CHANGE.port_index,
                                              status);
      }
      LINK_CHANGE.device_handle = NULL;
    }
    err = BmOK;
  }
  return err;
}

/*!
  @brief Create an ADIN2111 specific network device with required traits
         and callbacks
 */
static void create_network_device(void) {
  static NetworkDeviceTrait const trait = {
      .send = adin2111_netdevice_send_,
      .enable = adin2111_netdevice_enable_,
      .disable = adin2111_netdevice_disable_,
      .enable_port = adin2111_netdevice_enable_port_,
      .disable_port = adin2111_netdevice_disable_port_,
      .retry_negotiation = adin2111_netdevice_renegotiate_,
      .num_ports = adin2111_num_ports,
      .port_stats = adin2111_port_stats_,
      .handle_interrupt = adin2111_handle_interrupt};
  static NetworkDeviceCallbacks callbacks = {0};
  NETWORK_DEVICE.self = NULL;
  NETWORK_DEVICE.trait = &trait;
  NETWORK_DEVICE.callbacks = &callbacks;
}

/**************** Public API Functions ****************/
/*!
  @brief Initialize an Adin2111 device

  @return BmOK on success
  @return BmErr on failure
 */
BmErr adin2111_init(void) {
  BmErr err = BmOK;

  // Prevent allocating RX buffers more than once
  static bool initialized = false;
  if (initialized) {
    err = BmEALREADY;
    goto end;
  }
  initialized = true;

  for (int i = 0; i < RX_QUEUE_NUM_ENTRIES; i++) {
    adi_eth_BufDesc_t *buffer_description = &RX_BUFFERS[i];
    memset(buffer_description, 0, sizeof(adi_eth_BufDesc_t));
    buffer_description->pBuf =
        aligned_malloc(DMA_ALIGN_SIZE, MAX_FRAME_BUF_SIZE);
    if (!buffer_description->pBuf) {
      err = BmENOMEM;
      goto end;
    }
    memset(buffer_description->pBuf, 0, MAX_FRAME_BUF_SIZE);
    buffer_description->bufSize = MAX_FRAME_BUF_SIZE;
    buffer_description->cbFunc = receive_callback;
  }

  // set up the static memory
  create_network_device();
  err = adin2111_netdevice_enable();

end:
  return err;
}

/*!
  @brief Get a generic NetworkDevice for the Adin2111

  @return ADIN2111 network device
 */
NetworkDevice adin2111_network_device(void) {
  // set up the static memory
  create_network_device();
  return NETWORK_DEVICE;
}
