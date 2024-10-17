#include "helpers.hpp"
#include <gtest/gtest.h>
#include <stdbool.h>
#include <stdint.h>

#include "fff.h"

DEFINE_FFF_GLOBALS;

extern "C" {
#include "l2.h"
#include "mock_bm_ip.h"
#include "mock_bm_os.h"
}

struct L2Meta {
  BmL2RxCb rx;
  BmL2DeviceLinkChangeCb link_change;
  void *device_handle;
};

#define num_devices 3
#define num_port 6
#define port_per_device (num_port / num_devices)
#define egress_port_offset 51

#if (num_port % num_devices != 0)
#error "num_devices must divide evenly into num_port"
#endif

class l2_test : public ::testing::Test {
public:
  rnd_gen RND;
  static struct L2Meta META[num_devices];
  static uint8_t init_count;

  static BmErr init_cb(void *device_handle, BmL2RxCb rx_cb,
                       BmL2DeviceLinkChangeCb link_change_cb,
                       uint8_t port_mask) {
    BmErr err = BmOK;
    EXPECT_NE(device_handle, nullptr);
    EXPECT_NE(rx_cb, nullptr);
    EXPECT_NE(link_change_cb, nullptr);
    META[init_count].rx = rx_cb;
    META[init_count].link_change = link_change_cb;
    META[init_count].device_handle = device_handle;
    init_count++;
    return err;
  }

  static BmErr tx_cb(void *device_handle, uint8_t *buf, uint16_t size,
                     uint8_t port_mask, uint8_t port_offset) {
    BmErr err = BmOK;
    bool found = false;
    for (uint8_t i = 0; i < num_devices; i++) {
      if (device_handle == META[i].device_handle) {
        found = true;
      }
    }
    EXPECT_EQ(found, true);
    return err;
  }

  static BmErr power_cb(const void *device_handle, bool on, uint8_t port_mask) {
    BmErr err = BmOK;
    bool found = false;
    for (uint8_t i = 0; i < num_devices; i++) {
      if (device_handle == META[i].device_handle) {
        found = true;
      }
    }
    EXPECT_EQ(found, true);
    return err;
  }

private:
protected:
  l2_test() {}
  ~l2_test() override {}

  /*!
    @brief Create a configured number of devices and initialize driver
  */
  void SetUp() override {
    BmNetDevCfg cfg[num_devices];
    init_count = 0;

    for (uint8_t i = 0; i < num_devices; i++) {
      cfg[i].device_handle = (void *)RND.rnd_int(UINT64_MAX, UINT32_MAX);
      cfg[i].port_mask = 0b11;
      cfg[i].num_ports = port_per_device;
      cfg[i].init_cb = init_cb;
      cfg[i].power_cb = power_cb;
      cfg[i].tx_cb = tx_cb;
    }

    bm_queue_create_fake.return_val =
        (void *)RND.rnd_int(UINT32_MAX, UINT16_MAX);
    EXPECT_EQ(bm_l2_init(NULL, cfg, num_devices), BmOK);
  }
  void TearDown() override { bm_l2_deinit(); }
};

struct L2Meta l2_test::META[num_devices] = {};
uint8_t l2_test::init_count;

TEST_F(l2_test, init) {
  BmNetDevCfg cfg[num_devices];
  for (uint8_t i = 0; i < num_devices; i++) {
    cfg[i].device_handle = (void *)RND.rnd_int(UINT64_MAX, UINT32_MAX);
    cfg[i].port_mask = 0b11;
    cfg[i].num_ports = port_per_device;
    cfg[i].init_cb = init_cb;
    cfg[i].power_cb = power_cb;
    cfg[i].tx_cb = tx_cb;
  }

  // Test failures
  bm_l2_deinit();
  EXPECT_NE(bm_l2_init(NULL, NULL, num_devices), BmOK);
  EXPECT_NE(bm_l2_init(NULL, cfg, 0), BmOK);
  bm_task_create_fake.return_val = BmENOMEM;
  EXPECT_NE(bm_l2_init(NULL, cfg, num_devices), BmOK);
  bm_queue_create_fake.return_val = NULL;
  EXPECT_NE(bm_l2_init(NULL, cfg, num_devices), BmOK);
  bm_l2_deinit();
  RESET_FAKE(bm_task_create);
  RESET_FAKE(bm_queue_create);
}

/*!
  @brief Test the transmission wrapper of L2
*/
TEST_F(l2_test, link_output) {
  uint16_t size = RND.rnd_int(UINT16_MAX, UINT8_MAX);
  uint8_t *buf = (uint8_t *)bm_malloc(size);

  RND.rnd_array(buf, size);

  // Set egress port offset
  buf[egress_port_offset] = 1;

  bm_l2_get_payload_fake.return_val = buf;
  bm_queue_send_fake.return_val = BmOK;
  EXPECT_EQ(bm_l2_link_output(buf, size), BmOK);
  RESET_FAKE(bm_queue_send);

  // Set egress port offset (it is zeroed in bm_l2_link_output)
  buf[egress_port_offset] = 1;

  // Test failure
  bm_queue_send_fake.return_val = BmENOMEM;
  EXPECT_NE(bm_l2_link_output(buf, size), BmOK);
  RESET_FAKE(bm_queue_send);

  // Test egress port set to 0
  bm_queue_send_fake.return_val = BmOK;
  EXPECT_EQ(bm_l2_link_output(buf, size), BmOK);
  RESET_FAKE(bm_queue_send);

  RESET_FAKE(bm_l2_get_payload);

  bm_free(buf);
}

/*!
 @brief Get number of ports and devices configured for L2
*/
TEST_F(l2_test, num_devices_ports) {
  EXPECT_EQ(bm_l2_get_num_ports(), num_port);
  EXPECT_EQ(bm_l2_get_num_devices(), num_devices);
}

/*!
 @brief Get the device handle of all devices configured for L2
*/
TEST_F(l2_test, get_device_handle) {
  void *handle = NULL;
  uint32_t start_port_idx = 0;
  for (uint8_t i = 0; i < num_devices; i++) {
    EXPECT_EQ(bm_l2_get_device_handle(i, &handle, &start_port_idx), true);
    EXPECT_EQ(handle, META[i].device_handle);
    EXPECT_EQ(start_port_idx, i * (port_per_device));
  }

  // Test invalid use cases
  EXPECT_EQ(bm_l2_get_device_handle(0, NULL, NULL), false);
  EXPECT_EQ(bm_l2_get_device_handle(0, &handle, NULL), false);
  EXPECT_EQ(bm_l2_get_device_handle(0, NULL, &start_port_idx), false);
  EXPECT_EQ(bm_l2_get_device_handle(UINT8_MAX, &handle, &start_port_idx),
            false);
}

/*!
  @brief Test setting the power (on/off) of the all devices
*/
TEST_F(l2_test, set_power) {
  for (uint8_t i = 0; i < num_devices; i++) {
    EXPECT_EQ(bm_l2_netif_set_power(META[i].device_handle, true), BmOK);
    EXPECT_EQ(bm_l2_netif_set_power(META[i].device_handle, false), BmOK);
  }

  // Test invalid use cases
  EXPECT_NE(bm_l2_netif_set_power(NULL, true), BmOK);
}

/*!
 @brief Test the callbacks initialized by the L2 interface
*/
TEST_F(l2_test, test_callbacks) {
  uint16_t size = RND.rnd_int(UINT16_MAX, UINT8_MAX);
  uint8_t *buf = (uint8_t *)bm_malloc(size);
  RND.rnd_array(buf, size);

  bm_l2_new_fake.return_val = buf;
  bm_l2_get_payload_fake.return_val = buf;
  for (uint8_t i = 0; i < num_devices; i++) {
    ASSERT_EQ(META[i].rx(META[i].device_handle, buf, size,
                         RND.rnd_int(port_per_device, 1)),
              BmOK);
    META[i].link_change(META[i].device_handle, RND.rnd_int(port_per_device, 1),
                        true);
    META[i].link_change(META[i].device_handle, RND.rnd_int(port_per_device, 1),
                        false);
  }

  //Teset invalid use cases
  ASSERT_NE(META[0].rx(NULL, buf, size, RND.rnd_int(port_per_device, 1)), BmOK);
  bm_queue_send_fake.return_val = BmENOMEM;
  ASSERT_NE(META[0].rx(META[0].device_handle, buf, size,
                       RND.rnd_int(port_per_device, 1)),
            BmOK);
  bm_l2_new_fake.return_val = NULL;
  ASSERT_NE(META[0].rx(META[0].device_handle, buf, size,
                       RND.rnd_int(port_per_device, 1)),
            BmOK);
  ASSERT_NE(META[0].rx(META[0].device_handle, NULL, size,
                       RND.rnd_int(port_per_device, 1)),
            BmOK);
  META[0].link_change(NULL, RND.rnd_int(port_per_device, 1), true);

  bm_free(buf);
}

/*!
 @brief Get the state of the port (up/down)

 @details The port's state will be up due to the fact that the processing
          API in the thread is unreachable and therefore unable to disable
          the port
*/
TEST_F(l2_test, port_state) {
  bm_l2_get_port_state(RND.rnd_int(port_per_device, 1));
}
