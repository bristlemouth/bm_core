#ifndef __NETWORK_DEVICE_H__
#define __NETWORK_DEVICE_H__

#include "util.h"

typedef struct {
  void (*power)(bool on);
  void (*link_change)(uint8_t port_index, bool is_up);

  // Bristlemouth spec ingress port_num is an integer 1-15
  void (*receive)(uint8_t port_num, uint8_t *data, size_t length);
  void (*debug_packet_dump)(const uint8_t *data, size_t length);
} NetworkDeviceCallbacks;

typedef struct {
  // If port=0, send on all ports. Otherwise select a single egress port 1-15.
  BmErr (*const send)(void *self, uint8_t *data, size_t length, uint8_t port);
  BmErr (*const enable)(void *self);
  BmErr (*const disable)(void *self);
  BmErr (*const enable_port)(void *self, uint8_t port_num);
  BmErr (*const disable_port)(void *self, uint8_t port_num);
  BmErr (*const retry_negotiation)(void *self, uint8_t port_index, bool *renegotiated);
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
