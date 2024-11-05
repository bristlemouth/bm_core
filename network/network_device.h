#include "util.h"

typedef struct {
  void (*power)(bool on);
  void (*link_change)(uint8_t port_index, bool is_up);
  size_t (*receive)(uint8_t port_index, uint8_t *data, size_t length);
} NetworkDeviceCallbacks;

typedef struct {
  BmErr (*const send)(void *self, uint8_t *data, size_t length,
                      uint8_t port_mask);
  BmErr (*const enable)(void *self);
  BmErr (*const disable)(void *self);
} NetworkDeviceTrait;

typedef struct {
  void *self;
  NetworkDeviceTrait const *trait;
} NetworkDevice;
