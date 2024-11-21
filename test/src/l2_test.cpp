#include <gtest/gtest.h>
#include <helpers.hpp>
#include <stdbool.h>
#include <stdint.h>

#include "fff.h"

DEFINE_FFF_GLOBALS;

extern "C" {
#include "l2.h"
#include "mock_bm_ip.h"
#include "mock_bm_os.h"
#include "mock_bm_adin2111.h"
}

#define port_per_device 2
#define egress_port_offset 51

class L2 : public ::testing::Test {
public:
  rnd_gen RND;
  static uint8_t init_count;

  static BmErr tx_cb(void *device_handle, uint8_t *buf, uint16_t size,
                     uint8_t port_mask, uint8_t port_offset) {
    return BmOK;
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
