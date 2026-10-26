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
    // Queue is empty unless a test says otherwise
    bm_queue_receive_fake.return_val = BmENODATA;

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
  netdevice_enable_port_fake.return_val = BmOK;
  netdevice_disable_port_fake.return_val = BmOK;
  for (uint8_t i = 0; i < port_per_device; i++) {
    EXPECT_EQ(bm_l2_netif_enable_disable_port(i + 1, false), BmOK);
    EXPECT_EQ(bm_l2_netif_enable_disable_port(i + 1, true), BmOK);
  }

  // Test failures
  netdevice_enable_port_fake.return_val = BmENOMEM;
  netdevice_disable_port_fake.return_val = BmENOMEM;
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

/*!
 @brief Link change callbacks survive a stop and re-init
 */
TEST_F(L2, stop_keeps_link_change_callbacks) {
  EXPECT_EQ(bm_l2_register_link_change_callback(link_change_cb_1), BmOK);

  bm_l2_stop();
  EXPECT_EQ(bm_l2_init(network_device), BmOK);

  link_change_count = (struct LinkChangeCbCount){};
  network_device.callbacks->link_change(RND.rnd_int(port_per_device, 1), false);
  ASSERT_EQ(link_change_count.down_count, 1);
  ASSERT_EQ(link_change_count.up_count, 0);

  // Deinit clears the callbacks
  bm_l2_deinit();
  EXPECT_EQ(bm_l2_init(network_device), BmOK);
  link_change_count = (struct LinkChangeCbCount){};
  network_device.callbacks->link_change(RND.rnd_int(port_per_device, 1), false);
  ASSERT_EQ(link_change_count.down_count, 0);
}

static uint8_t queued_evt[64];
static bool queued_evt_pending;

static BmErr capture_queue_send(BmQueue queue, const void *item,
                                uint32_t timeout_ms) {
  (void)queue;
  (void)timeout_ms;
  memcpy(queued_evt, item, bm_queue_create_fake.arg1_val);
  queued_evt_pending = true;
  return BmOK;
}

static BmErr replay_queue_receive(BmQueue queue, void *item,
                                  uint32_t timeout_ms) {
  (void)queue;
  (void)timeout_ms;
  if (!queued_evt_pending) {
    return BmENODATA;
  }
  memcpy(item, queued_evt, bm_queue_create_fake.arg1_val);
  queued_evt_pending = false;
  return BmOK;
}

static BmErr set_task_handle(BmTaskCb task, const char *name,
                             uint32_t stack_size, void *arg, uint32_t priority,
                             BmTaskHandle task_handle) {
  (void)task;
  (void)name;
  (void)stack_size;
  (void)arg;
  (void)priority;
  *(BmTaskHandle *)task_handle = (BmTaskHandle)0x1234;
  return BmOK;
}

/*!
 @brief Stop deletes the task before the queue and frees queued TX buffers
 */
TEST_F(L2, stop_frees_queued_tx) {
  // Re-init so the L2 task has a handle to delete
  bm_l2_stop();
  bm_task_create_fake.custom_fake = set_task_handle;
  EXPECT_EQ(bm_l2_init(network_device), BmOK);
  RESET_FAKE(bm_task_create);

  ASSERT_LE(bm_queue_create_fake.arg1_val, sizeof(queued_evt));
  uint8_t buf[128] = {0};
  bm_l2_get_payload_fake.return_val = buf;

  queued_evt_pending = false;
  bm_queue_send_fake.custom_fake = capture_queue_send;
  EXPECT_EQ(bm_l2_link_output(buf, sizeof(buf)), BmOK);
  ASSERT_TRUE(queued_evt_pending);

  RESET_FAKE(bm_l2_free);
  RESET_FAKE(bm_task_delete);
  RESET_FAKE(bm_queue_delete);
  FFF_RESET_HISTORY();
  bm_queue_receive_fake.custom_fake = replay_queue_receive;

  bm_l2_stop();

  EXPECT_FALSE(queued_evt_pending);
  EXPECT_EQ(bm_l2_free_fake.call_count, 1);
  EXPECT_EQ(bm_l2_free_fake.arg0_val, buf);
  EXPECT_EQ(bm_task_delete_fake.call_count, 1);
  EXPECT_EQ(bm_queue_delete_fake.call_count, 1);

  // Task must be deleted before the queue it blocks on
  int task_idx = -1;
  int queue_idx = -1;
  for (unsigned int i = 0; i < fff.call_history_idx; i++) {
    if (fff.call_history[i] == (void *)bm_task_delete) {
      task_idx = i;
    } else if (fff.call_history[i] == (void *)bm_queue_delete) {
      queue_idx = i;
    }
  }
  EXPECT_GE(task_idx, 0);
  EXPECT_GT(queue_idx, task_idx);

  // Late interrupts fail safely once stopped
  EXPECT_EQ(bm_l2_handle_device_interrupt(), BmENOMEM);

  RESET_FAKE(bm_queue_send);
  RESET_FAKE(bm_queue_receive);
  RESET_FAKE(bm_l2_get_payload);
  bm_queue_receive_fake.return_val = BmENODATA;
}

/*!
 @brief Tset link local routing callback assignment
 */
TEST_F(L2, register_link_local_routing_callback) {

  L2LinkLocalRoutingCb cb = (L2LinkLocalRoutingCb)RND.rnd_int(UINT32_MAX, 1);

  EXPECT_EQ(bm_l2_register_link_local_routing_callback(NULL), BmEINVAL);
  EXPECT_EQ(bm_l2_register_link_local_routing_callback(cb), BmOK);
}
