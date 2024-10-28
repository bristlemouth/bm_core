#include <gtest/gtest.h>
#include <helpers.hpp>

#include "fff.h"

DEFINE_FFF_GLOBALS;

extern "C" {
#include "mock_bm_service_request.h"
#include "sys_info_service.h"
}

class SysInfoService : public ::testing::Test {
public:
  rnd_gen RND;

private:
protected:
  SysInfoService() {}
  ~SysInfoService() override {}

  void SetUp() override {}
  void TearDown() override {}
  static bool service_reply_cb(bool ack, uint32_t msg_id, size_t service_strlen,
                               const char *service, size_t reply_len,
                               uint8_t *reply_data) {
    (void)ack;
    (void)msg_id;
    (void)service_strlen;
    (void)service;
    (void)reply_len;
    (void)reply_data;

    return true;
  }
};

TEST_F(SysInfoService, init) { sys_info_service_init(); }

TEST_F(SysInfoService, request) {
  uint64_t node_id = RND.rnd_int(UINT32_MAX, 1);
  uint32_t timeout = 10;

  bm_service_request_fake.return_val = true;
  EXPECT_EQ(sys_info_service_request(node_id, service_reply_cb, timeout), true);

  bm_service_request_fake.return_val = false;
  EXPECT_EQ(sys_info_service_request(node_id, service_reply_cb, timeout),
            false);
}
