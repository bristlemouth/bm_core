#ifndef __NETWORK_DEVICE_H__
#define __NETWORK_DEVICE_H__

#include "util.h"

typedef struct {
  void (*power)(bool on);
  void (*link_change)(uint8_t port_index, bool is_up);
  void (*receive)(uint8_t port_mask, uint8_t *data, size_t length);
  void (*debug_packet_dump)(const uint8_t *data, size_t length);
} NetworkDeviceCallbacks;

typedef struct {
  BmErr (*const send)(void *self, uint8_t *data, size_t length, uint8_t port);
  BmErr (*const enable)(void *self);
  BmErr (*const disable)(void *self);
  uint8_t (*const num_ports)(void);
  BmErr (*const port_stats)(void *self, uint8_t port_index, void *stats);
  BmErr (*const handle_interrupt)(void *self);
} NetworkDeviceTrait;

typedef struct {
  void *self;
  NetworkDeviceTrait const *trait;
  NetworkDeviceCallbacks *callbacks;
} NetworkDevice;

#endif // __NETWORK_DEVICE_H__
