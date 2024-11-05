#include "bm_adin2111.h"
#include "aligned_malloc.h"
#include "bm_os.h"
#include <stdbool.h>
#include <string.h>

// Extra 4 bytes for FCS and 2 bytes for the frame header
#define MAX_FRAME_BUF_SIZE (MAX_FRAME_SIZE + 4 + 2)
#define DMA_ALIGN_SIZE (4)

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

/**************** Private Helper Functions ****************/

// Called by the driver when the link status changes
// If the user has registered a callback, call it
static void link_change_callback_(void *device_handle, uint32_t event,
                                  void *status_registers_param) {
  (void)event;

  if (NETWORK_DEVICE.callbacks.link_change) {
    const adi_mac_StatusRegisters_t *status_registers =
        (adi_mac_StatusRegisters_t *)status_registers_param;

    int port_index = -1;
    if (status_registers->p1StatusMasked == ADI_PHY_EVT_LINK_STAT_CHANGE) {
      port_index = ADIN2111_PORT_1;
    } else if (status_registers->p2StatusMasked ==
               ADI_PHY_EVT_LINK_STAT_CHANGE) {
      port_index = ADIN2111_PORT_2;
    }

    if (port_index >= 0) {
      adi_eth_LinkStatus_e status;
      adi_eth_Result_e result =
          adin2111_GetLinkStatus(device_handle, port_index, &status);
      if (result == ADI_ETH_SUCCESS) {
        NETWORK_DEVICE.callbacks.link_change(port_index, status);
      }
    }
  }
}

// Start up and enable the ADIN2111 hardware
static BmErr adin2111_netdevice_enable(void) {
  BmErr err = BmOK;

  if (NETWORK_DEVICE.callbacks.power) {
    NETWORK_DEVICE.callbacks.power(true);
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
  if (err != BmOK && NETWORK_DEVICE.callbacks.power) {
    NETWORK_DEVICE.callbacks.power(false);
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

  if (NETWORK_DEVICE.callbacks.power) {
    NETWORK_DEVICE.callbacks.power(false);
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
  buffer_description->bufSize = length;
  buffer_description->cbFunc = tx_complete;
  adin2111_TxPort_e tx_port = ADIN2111_TX_PORT_FLOOD;
  if (port_mask == 1) {
    tx_port = ADIN2111_TX_PORT_1;
  } else if (port_mask == 2) {
    tx_port = ADIN2111_TX_PORT_2;
  }
  adin2111_SubmitTxBuffer(&DEVICE_STRUCT, tx_port, buffer_description);
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

  if (NETWORK_DEVICE.callbacks.receive) {
    adi_eth_BufDesc_t *buffer_description =
        (adi_eth_BufDesc_t *)buffer_description_param;
    uint8_t port_mask = 1 << buffer_description->port;
    NETWORK_DEVICE.callbacks.receive(port_mask, buffer_description->pBuf,
                                     buffer_description->bufSize);
  }
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

  err = adin2111_netdevice_enable();

end:
  return err;
}

/// Build a generic NetworkDevice for the Adin2111
NetworkDevice create_adin2111_network_device(void) {
  // Create the vtable once and attach a pointer to it every time
  static NetworkDeviceTrait const trait = {.send = adin2111_netdevice_send_,
                                           .enable = adin2111_netdevice_enable_,
                                           .disable =
                                               adin2111_netdevice_disable_};
  NETWORK_DEVICE.trait = &trait;
  NETWORK_DEVICE.callbacks = (NetworkDeviceCallbacks){0};
  return NETWORK_DEVICE;
}
