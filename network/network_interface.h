#include "util.h"

typedef struct {
  void (*power)(bool on);
  void (*link_change)(uint8_t port_index, bool is_up);
  size_t (*receive)(uint8_t port_index, uint8_t *data, size_t length);
} NetworkInterfaceCallbacks;

typedef struct {
  BmErr (*const send)(void *self, uint8_t *data, size_t length);
  BmErr (*const enable)(void *self);
  BmErr (*const disable)(void *self);
} NetworkInterfaceTrait;

typedef struct {
  void *self;
  NetworkInterfaceTrait const *trait;
} NetworkInterface;
