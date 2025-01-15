#include <gtest/gtest.h>
#include <helpers.hpp>
#include <stdbool.h>
#include <stdint.h>

#include "fff.h"

DEFINE_FFF_GLOBALS;

extern "C" {
#include "l2.h"
#include "mock_bm_adin2111.h"
#include "mock_bm_ip.h"
#include "mock_bm_os.h"
}

#define port_per_device 2
#define egress_port_offset 51

static struct LinkChangeCbCount {
  uint8_t down_count;
  uint8_t up_count;
} link_change_count;

class L2 : public ::testing::Test {
public:
  rnd_gen RND;
  static uint8_t init_count;

  static BmErr tx_cb(void *device_handle, uint8_t *buf, uint16_t size,
                     uint8_t port_mask, uint8_t port_offset) {
    return BmOK;
  }
  static inline void link_change_common_cb(uint8_t state) {
    if (state) {
      link_change_count.up_count++;
    } else {
      link_change_count.down_count++;
    }
  }
  static void link_change_cb_1(uint8_t port, bool state) {
    (void)port;
    link_change_common_cb(state);
  }
  static void link_change_cb_2(uint8_t port, bool state) {
    (void)port;
    link_change_common_cb(state);
  }
  static void link_change_cb_3(uint8_t port, bool state) {
    (void)port;
    link_change_common_cb(state);
  }

private:
protected:
  L2() {}
  ~L2() override {}

  NetworkDevice network_device;

  void SetUp() override {
    netdevice_enable_fake.return_val = BmOK;
    netdevice_disable_fake.return_val = BmOK;
    network_device = adin2111_network_device();
    init_count = 0;
    bm_queue_create_fake.return_val =
        (void *)RND.rnd_int(UINT32_MAX, UINT16_MAX);
    netdevice_num_ports_fake.return_val = port_per_device;

    EXPECT_EQ(bm_l2_init(network_device), BmOK);
  }
  void TearDown() override { bm_l2_deinit(); }
};

uint8_t L2::init_count;

TEST_F(L2, init) {
  // Test failures
  bm_l2_deinit();
  // Reset init count after deinit
  init_count = 0;
  bm_task_create_fake.return_val = BmENOMEM;
  EXPECT_NE(bm_l2_init(network_device), BmOK);
  bm_l2_deinit();
  // Reset init count after deinit
  init_count = 0;
  bm_queue_create_fake.return_val = NULL;
  EXPECT_NE(bm_l2_init(network_device), BmOK);
  bm_l2_deinit();
  // Reset init count after deinit
  init_count = 0;
  RESET_FAKE(bm_task_create);
  RESET_FAKE(bm_queue_create);
}

/*!
  @brief Test the transmission wrapper of L2
*/
TEST_F(L2, link_output) {
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
  @brief Test setting the power (on/off) of the all devices
*/
TEST_F(L2, set_power) {
  EXPECT_EQ(bm_l2_netif_set_power(true), BmOK);
  EXPECT_EQ(bm_l2_netif_set_power(false), BmOK);
}

/*!
 @brief Get the state of the port (up/down)

 @details The port's state will be up due to the fact that the processing
          API in the thread is unreachable and therefore unable to disable
          the port
*/
TEST_F(L2, port_state) {
  bm_l2_get_port_state(RND.rnd_int(port_per_device, 1));
}

/*!
 @brief Disable/enable ports
 */
TEST_F(L2, enable_disable_ports) {
  netdevice_enable_fake.return_val = BmOK;
  netdevice_disable_fake.return_val = BmOK;
  for (uint8_t i = 0; i < port_per_device; i++) {
    EXPECT_EQ(bm_l2_netif_enable_disable_port(i + 1, false), BmOK);
    EXPECT_EQ(bm_l2_netif_enable_disable_port(i + 1, true), BmOK);
  }

  // Test failures
  netdevice_enable_fake.return_val = BmENOMEM;
  netdevice_disable_fake.return_val = BmENOMEM;
  for (uint8_t i = 0; i < port_per_device; i++) {
    EXPECT_NE(bm_l2_netif_enable_disable_port(i + 1, false), BmOK);
    EXPECT_NE(bm_l2_netif_enable_disable_port(i + 1, true), BmOK);
  }
  EXPECT_NE(bm_l2_netif_enable_disable_port(port_per_device + 1, true), BmOK);
}

/*!
 @brief Exercise link change callbacks
 */
TEST_F(L2, link_change) {
  EXPECT_EQ(bm_l2_register_link_change_callback(link_change_cb_1), BmOK);
  EXPECT_EQ(bm_l2_register_link_change_callback(link_change_cb_2), BmOK);
  EXPECT_EQ(bm_l2_register_link_change_callback(link_change_cb_3), BmOK);
  EXPECT_NE(bm_l2_register_link_change_callback(NULL), BmOK);

  link_change_count = (struct LinkChangeCbCount){};
  network_device.callbacks->link_change(RND.rnd_int(port_per_device, 1), false);
  ASSERT_EQ(link_change_count.down_count, 3);
  ASSERT_EQ(link_change_count.up_count, 0);

  link_change_count = (struct LinkChangeCbCount){};
  network_device.callbacks->link_change(RND.rnd_int(port_per_device, 1), true);
  ASSERT_EQ(link_change_count.down_count, 0);
  ASSERT_EQ(link_change_count.up_count, 3);
}
