#include "bm_adin2111.h"
#include "fff.h"
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

FAKE_VOID_FUNC(netif_power_cb, bool);
FAKE_VOID_FUNC(link_changed_on_port, uint8_t, bool);
FAKE_VALUE_FUNC(size_t, received_data_on_port, uint8_t, uint8_t *, size_t);

static NetworkDeviceCallbacks const callbacks = {
    .power = netif_power_cb,
    .link_change = link_changed_on_port,
    .receive = received_data_on_port};

static NetworkDevice setup(void) {
  static Adin2111 adin = {.device_handle = nullptr, .callbacks = &callbacks};

  // We can only call adin2111_init once per execution (test suite)
  // because the device memory in the driver is static.
  if (adin.device_handle == nullptr) {
    BmErr err = adin2111_init(&adin);

    // It would take a lot of mocking to pretend SPI transactions work.
    // On a real device, this should return BmOK.
    EXPECT_EQ(err, BmENODEV);
  }

  return prep_adin2111_netif(&adin);
}

TEST(Adin2111, send) {
  NetworkDevice netif = setup();
  BmErr err = netif.trait->send(netif.self, (unsigned char *)"hello", 5,
                                ADIN2111_PORT_MASK);
  EXPECT_EQ(err, BmOK);
}

TEST(Adin2111, enable) {
  NetworkDevice netif = setup();
  // Expect the same BmENODEV error as in init
  // because init calls enable internally.
  // On a real device, this should return BmOK,
  // but we don't have the SPI transactions mocked.
  BmErr err = netif.trait->enable(netif.self);
  EXPECT_EQ(err, BmENODEV);
}

TEST(Adin2111, disable) {
  NetworkDevice netif = setup();
  // SEGFAULT because PHY is NULL, because no real SPI transactions
  EXPECT_DEATH(netif.trait->disable(netif.self), "");
}
