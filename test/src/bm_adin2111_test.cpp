#include "fff.h"
#include "mock_bm_adin2111.h"
#include "gtest/gtest.h"

DEFINE_FFF_GLOBALS;

extern "C" {
void *bm_malloc(size_t size) { return malloc(size); }
void bm_free(void *p) { free(p); }
}

FAKE_VALUE_FUNC(uint32_t, HAL_DisableIrq);
FAKE_VALUE_FUNC(uint32_t, HAL_EnableIrq);
FAKE_VALUE_FUNC(uint32_t, HAL_GetEnableIrq);
FAKE_VALUE_FUNC(uint32_t, HAL_RegisterCallback, HAL_Callback_t const *, void *);
FAKE_VALUE_FUNC(uint32_t, HAL_SpiReadWrite, uint8_t *, uint8_t *, uint32_t,
                bool);
FAKE_VALUE_FUNC(uint32_t, HAL_SpiRegisterCallback, HAL_Callback_t const *,
                void *);
FAKE_VALUE_FUNC(uint32_t, HAL_UnInit_Hook);
FAKE_VALUE_FUNC(long unsigned int, __REV, long unsigned int);
FAKE_VOID_FUNC(__disable_irq);
FAKE_VOID_FUNC(__enable_irq);

static NetworkDevice setup() {
  adin2111_init();
  return adin2111_network_device();
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

TEST(Adin2111, set_power_cb_before_init) {
  NetworkDevice device = adin2111_network_device();
  device.callbacks->power = network_device_power_cb;
  RESET_FAKE(network_device_power_cb);
  EXPECT_EQ(network_device_power_cb_fake.call_count, 0);
  adin2111_init();
  // Called twice because we first turn the adin on,
  // then when we get SPI errors, we turn it off
  EXPECT_EQ(network_device_power_cb_fake.call_count, 2);
}
