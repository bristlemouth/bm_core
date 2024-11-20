#include "bm_adin2111.h"
#include "fff.h"
#include "gtest/gtest.h"

DEFINE_FFF_GLOBALS;

extern "C" {
void *bm_malloc(size_t size) { return malloc(size); }
void bm_free(void *p) { free(p); }

FAKE_VALUE_FUNC(uint32_t, __REV, uint32_t);
FAKE_VOID_FUNC(__disable_irq);
FAKE_VOID_FUNC(__enable_irq);
}

FAKE_VOID_FUNC(network_device_power_cb, bool);

FAKE_VALUE_FUNC(uint32_t, HAL_DisableIrq);
FAKE_VALUE_FUNC(uint32_t, HAL_EnableIrq);
FAKE_VALUE_FUNC(uint32_t, HAL_GetEnableIrq);
FAKE_VALUE_FUNC(uint32_t, HAL_SpiReadWrite, uint8_t *, uint8_t *, uint32_t,
                bool);
FAKE_VALUE_FUNC(uint32_t, HAL_SpiRegisterCallback, HAL_Callback_t const *,
                void *);
FAKE_VALUE_FUNC(uint32_t, HAL_UnInit_Hook);

static NetworkDevice setup() {
  NetworkDevice device = adin2111_network_device();
  device.callbacks->power = network_device_power_cb;
  adin2111_init();
  return device;
}

TEST(Adin2111, send) {
  NetworkDevice device = setup();
  BmErr err = device.trait->send(device.self, (unsigned char *)"hello", 5,
                                 ADIN2111_PORT_MASK);
  EXPECT_EQ(err, BmOK);
}

TEST(Adin2111, enable) {
  NetworkDevice device = setup();
  BmErr err = device.trait->enable(device.self);
  // We're exercising the embedded driver code,
  // but there's no real SPI device on the bus.
  EXPECT_EQ(err, BmENODEV);
}

TEST(Adin2111, disable) {
  NetworkDevice device = setup();
  // SEGFAULT because PHY is NULL, because no real SPI transactions
  EXPECT_DEATH(device.trait->disable(device.self), "");
}

TEST(Adin2111, num_ports) {
  NetworkDevice device = setup();
  EXPECT_EQ(device.trait->num_ports(), ADIN2111_PORT_NUM);
}

TEST(Adin2111, set_power_cb_before_init) {
  setup();
  // Called twice because we first turn the adin on,
  // then when we get SPI errors, we turn it off
  EXPECT_EQ(network_device_power_cb_fake.call_count, 2);
}

TEST(Adin2111, port_stats) {
  NetworkDevice device = setup();
  Adin2111PortStats stats;
  for (int port = 0; port < device.trait->num_ports(); port++) {
    // SEGFAULT because PHY is NULL, because no real SPI transactions
    EXPECT_DEATH(device.trait->port_stats(device.self, port, &stats), "");
  }
}
