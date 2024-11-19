#include "bm_adin2111.h"
#include "aligned_malloc.h"
#include "bm_config.h"
#include "bm_network_generic.h"
#include "bm_os.h"
#include <stdbool.h>
#include <string.h>

// Extra 4 bytes for FCS and 2 bytes for the frame header
#define MAX_FRAME_BUF_SIZE (MAX_FRAME_SIZE + 4 + 2)
#define DMA_ALIGN_SIZE (4)
#define NO_LINK_CHANGE (-1)

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
static HAL_Callback_t ADIN2111_MAC_SPI_CALLBACK = NULL;
static void *ADIN2111_MAC_SPI_CALLBACK_PARAM = NULL;
static struct LinkChange LINK_CHANGE = {NULL, ADIN2111_PORT_1};

/**************** Private Helper Functions ****************/

// Required by the Analog Devices adi_hal.h to be implementeed by the application.
// The user will implement the bm_network_spi_read_write function in their application.
uint32_t HAL_SpiReadWrite(uint8_t *pBufferTx, uint8_t *pBufferRx,
                          uint32_t nbBytes, bool useDma) {

  uint32_t ret =
      bm_network_spi_read_write(pBufferTx, pBufferRx, nbBytes, useDma) == BmOK
          ? 0
          : 1;
  if (ret == 0) {
    if (ADIN2111_MAC_SPI_CALLBACK) {
      ADIN2111_MAC_SPI_CALLBACK(ADIN2111_MAC_SPI_CALLBACK_PARAM, 0, NULL);
    }
  } else {
    bm_debug("Network SPI Read/Write Failed\n");
  }

  return ret;
}

// Required by the Analog Devices adi_hal.h to be implementeed by the application.
// We save the pointer here in our driver wrapper to simplify Bristlemouth integration.
uint32_t HAL_RegisterCallback(HAL_Callback_t const *intCallback,
                              void *hDevice) {
  // Analog Devices code has a bug at adi_mac.c:633 where they
  // cast a function pointer to a function pointer pointer incorrectly.
  ADIN2111_MAC_INT_CALLBACK = (const HAL_Callback_t)intCallback;
  ADIN2111_MAC_INT_CALLBACK_PARAM = hDevice;
  return ADI_ETH_SUCCESS;
}

// Required by the Analog Devices adi_hal.h to be implementeed by the application.
// We save the pointer here in our driver wrapper to simplify Bristlemouth integration.
uint32_t HAL_SpiRegisterCallback(HAL_Callback_t const *spiCallback,
                                 void *hDevice) {
  // Analog Devices code has a bug at adi_mac.c:535 where they
  // cast a function pointer to a function pointer pointer incorrectly.
  ADIN2111_MAC_SPI_CALLBACK = (const HAL_Callback_t)spiCallback;
  ADIN2111_MAC_SPI_CALLBACK_PARAM = hDevice;
  return ADI_ETH_SUCCESS;
}

// Called by the driver when the link status changes
// If the user has registered a callback, call it
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

// Start up and enable the ADIN2111 hardware
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

  for (int i = 0; i < ADIN2111_PORT_NUM; i++) {
    result = adin2111_EnablePort(&DEVICE_STRUCT, i);
    if (result != ADI_ETH_SUCCESS) {
      err = BmENODEV;
      break;
    }
  }

end:
  if (err != BmOK && NETWORK_DEVICE.callbacks->power) {
    NETWORK_DEVICE.callbacks->power(false);
  }

  return err;
}

// Trait wrapper function to convert self from void* to Adin2111*
inline static BmErr adin2111_netdevice_enable_(void *self) {
  (void)self;
  return adin2111_netdevice_enable();
}

// Shut down and disable the ADIN2111 hardware
static BmErr adin2111_netdevice_disable(void) {
  BmErr err = BmOK;

  for (int i = 0; i < ADIN2111_PORT_NUM; i++) {
    adi_eth_Result_e result = adin2111_DisablePort(&DEVICE_STRUCT, i);
    if (result != ADI_ETH_SUCCESS) {
      err = BmENODEV;
      break;
    }
  }

  if (NETWORK_DEVICE.callbacks->power) {
    NETWORK_DEVICE.callbacks->power(false);
  }

  return err;
}

// Trait wrapper function to convert self from void* to Adin2111*
inline static BmErr adin2111_netdevice_disable_(void *self) {
  (void)self;
  return adin2111_netdevice_disable();
}

// After a TX buffer is sent, it gets freed here
static void free_tx_buffer(adi_eth_BufDesc_t *buffer_description) {
  if (buffer_description) {
    if (buffer_description->pBuf) {
      aligned_free(buffer_description->pBuf);
    }
    bm_free(buffer_description);
  }
}

// This is the callback that the driver calls after a TX buffer is sent
static void tx_complete(void *device_param, uint32_t event,
                        void *buffer_description) {
  (void)device_param;
  (void)event;

  free_tx_buffer(buffer_description);
}

// Allocate buffers for sending, copy the given data, and submit to the driver
static BmErr adin2111_netdevice_send(uint8_t *data, size_t length,
                                     uint8_t port_mask) {
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
  adin2111_TxPort_e tx_port = ADIN2111_TX_PORT_FLOOD;
  if (port_mask == 1) {
    tx_port = ADIN2111_TX_PORT_1;
  } else if (port_mask == 2) {
    tx_port = ADIN2111_TX_PORT_2;
  }
  if (adin2111_SubmitTxBuffer(&DEVICE_STRUCT, tx_port, buffer_description) !=
      ADI_ETH_SUCCESS) {
    free_tx_buffer(buffer_description);
  }

end:
  return err;
}

// Trait wrapper function to convert self from void* to Adin2111*
static inline BmErr adin2111_netdevice_send_(void *self, uint8_t *data,
                                             size_t length, uint8_t port_mask) {
  (void)self;
  return adin2111_netdevice_send(data, length, port_mask);
}

// Called by the driver on received data
// If the user has registered a callback, call it
static void receive_callback_(void *device, uint32_t event,
                              void *buffer_description_param) {
  (void)device;
  (void)event;
  adi_eth_BufDesc_t *buffer_description = NULL;

  if (NETWORK_DEVICE.callbacks->receive) {
    buffer_description = (adi_eth_BufDesc_t *)buffer_description_param;
    uint8_t port_mask = 1 << buffer_description->port;
    NETWORK_DEVICE.callbacks->receive(port_mask, buffer_description->pBuf,
                                      buffer_description->trxSize);
  }

  // Re-submit buffer into ADIN's RX queue
  adi_eth_Result_e result =
      adin2111_SubmitRxBuffer(&DEVICE_STRUCT, buffer_description);
  if (result != ADI_ETH_SUCCESS) {
    bm_debug("Unable to re-submit RX Buffer\n");
  }
}

static inline uint8_t adin2111_num_ports(void) { return ADIN2111_PORT_NUM; }

// Get device-specific statistics for a port
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

// Trait wrapper function to convert void* to device-specific types
static inline BmErr adin2111_port_stats_(void *self, uint8_t port_index,
                                         void *stats) {
  (void)self;
  return adin2111_port_stats(port_index, stats);
}

// L2 calls this trait function from a thread
// after getting out of the external pin interrupt context.
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

static void create_network_device(void) {
  static NetworkDeviceTrait const trait = {
      .send = adin2111_netdevice_send_,
      .enable = adin2111_netdevice_enable_,
      .disable = adin2111_netdevice_disable_,
      .num_ports = adin2111_num_ports,
      .port_stats = adin2111_port_stats_,
      .handle_interrupt = adin2111_handle_interrupt};
  static NetworkDeviceCallbacks callbacks = {0};
  NETWORK_DEVICE.self = NULL;
  NETWORK_DEVICE.trait = &trait;
  NETWORK_DEVICE.callbacks = &callbacks;
}

/**************** Public API Functions ****************/

/*! @brief Initialize an Adin2111 device
    @return BmOK if successful, otherwise an error */
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
    buffer_description->cbFunc = receive_callback_;
  }

  // set up the static memory
  create_network_device();
  err = adin2111_netdevice_enable();

end:
  return err;
}

/// Get a generic NetworkDevice for the Adin2111
NetworkDevice adin2111_network_device(void) {
  // set up the static memory
  create_network_device();
  return NETWORK_DEVICE;
}
