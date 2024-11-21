#include "fff.h"
#include <gtest/gtest.h>

DEFINE_FFF_GLOBALS;

extern "C" {
#include "bristlemouth.h"
#include "mock_bm_adin2111.h"

FAKE_VALUE_FUNC(BmErr, bm_middleware_init, uint16_t);
}

FAKE_VOID_FUNC(netdev_power_cb, bool);

TEST(Bristlemouth, init) {
  BmErr err = bristlemouth_init(netdev_power_cb);
  EXPECT_EQ(err, BmOK);
}

TEST(Bristlemouth, network_device) {
  NetworkDevice network_device = bristlemouth_network_device();
   EXPECT_NE(network_device.trait, nullptr);
}