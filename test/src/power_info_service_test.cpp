#include <gtest/gtest.h>
#include <helpers.hpp>

#include "fff.h"

DEFINE_FFF_GLOBALS;

extern "C" {
#include "mock_bm_service.h"
#include "mock_bm_service_request.h"
#include "mock_cb_queue.h"
}

#include "power_info_service.h"

class PowerInfoService : public ::testing::Test {

protected:
  PowerInfoService() {}
  ~PowerInfoService() override {}
  void SetUp() override {}
  void TearDown() override {}
  static PowerInfoReplyData data_cb(void *arg) {
    PowerInfoReplyData d = {};
    return d;
  }
};

TEST_F(PowerInfoService, init) {
  // Test initialization failures
  ASSERT_EQ(power_info_service_init(NULL, NULL), BmEINVAL);

  bm_service_register_fake.return_val = false;
  ASSERT_EQ(power_info_service_init(data_cb, NULL), BmECANCELED);
  RESET_FAKE(bm_service_register);

  // Test initialization success
  bm_service_register_fake.return_val = true;
  ASSERT_EQ(power_info_service_init(data_cb, NULL), BmOK);
  RESET_FAKE(bm_service_register);
}

TEST_F(PowerInfoService, request) {
  // Test request failures
  queue_cb_enqueue_fake.return_val = BmEINVAL;
  EXPECT_EQ(power_info_service_request(NULL, 1), BmEINVAL);
  RESET_FAKE(queue_cb_enqueue);
  queue_cb_enqueue_fake.return_val = BmOK;
  bm_service_request_fake.return_val = false;
  EXPECT_EQ(power_info_service_request(NULL, 1), BmEBADMSG);
  RESET_FAKE(queue_cb_enqueue);
  RESET_FAKE(bm_service_request);

  // Test request success
  queue_cb_enqueue_fake.return_val = BmOK;
  bm_service_request_fake.return_val = true;
  EXPECT_EQ(power_info_service_request(NULL, 1), BmOK);
  RESET_FAKE(queue_cb_enqueue);
  RESET_FAKE(bm_service_request);
}
