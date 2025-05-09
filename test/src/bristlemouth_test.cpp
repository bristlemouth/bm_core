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
  DeviceCfg device = {
      .node_id = 0,
      .git_sha = 0,
      .device_name = "none",
      .version_string = "none",
      .vendor_id = 0,
      .product_id = 0,
      .hw_ver = 0,
      .ver_major = 0,
      .ver_minor = 0,
      .ver_patch = 0,
      .sn = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c',
             'd', 'e', 'f'},
  };
  BmErr err = bristlemouth_init(netdev_power_cb, device);
  EXPECT_EQ(err, BmOK);
}

TEST(Bristlemouth, network_device) {
  NetworkDevice network_device = bristlemouth_network_device();
  EXPECT_NE(network_device.trait, nullptr);
}
