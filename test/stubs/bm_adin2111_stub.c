#include "bm_adin2111.h"
#include "fff.h"

DEFINE_FFF_GLOBALS;

FAKE_VOID_FUNC(network_device_power_cb, bool);
FAKE_VOID_FUNC(link_changed_on_port, uint8_t, bool);
FAKE_VALUE_FUNC(size_t, received_data_on_port, uint8_t, uint8_t *, size_t);

static NetworkDeviceCallbacks const callbacks = {
    .power = network_device_power_cb,
    .link_change = link_changed_on_port,
    .receive = received_data_on_port};

NetworkDevice create_fake_network_device(void) {
  static Adin2111 adin = {.device_handle = NULL, .callbacks = &callbacks};

  // We can only call adin2111_init once per execution (test suite)
  // because the device memory in the driver is static.
  if (adin.device_handle == NULL) {
    adin2111_init(&adin);
  }

  return create_adin2111_network_device(&adin);
}
