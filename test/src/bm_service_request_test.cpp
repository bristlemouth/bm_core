#include <gtest/gtest.h>
#include <helpers.hpp>
#include <string.h>

#include "fff.h"

DEFINE_FFF_GLOBALS;

extern "C" {
#include "bm_service_common.h"
#include "bm_service_request.h"
#include "mock_bm_os.h"
#include "mock_pubsub.h"
}

class BmServiceRequest : public ::testing::Test {
public:
  rnd_gen RND;

private:
protected:
  const char *service = "/rand/service/name";
  BmServiceRequest() {}
  ~BmServiceRequest() override {}

  void SetUp() override {}
  void TearDown() override {}
  static bool service_handler(bool ack, uint32_t msg_id, size_t service_strlen,
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

TEST_F(BmServiceRequest, init) { bm_service_request_init(); }

TEST_F(BmServiceRequest, request) {
  size_t size = RND.rnd_int(UINT8_MAX, UINT8_MAX / 2);
  uint8_t *data = (uint8_t *)bm_malloc(size);
  RND.rnd_array(data, size);
  bm_pub_wl_fake.return_val = true;
  bm_sub_wl_fake.return_val = true;
  ASSERT_EQ(bm_service_request(strlen(service), service, size,
                               (const uint8_t *)data, service_handler, 1000),
            true);

  // Test failure cases
  ASSERT_EQ(bm_service_request(strlen(service), service,
                               MAX_BM_SERVICE_DATA_SIZE + 1,
                               (const uint8_t *)data, service_handler, 1000),
            false);
  bm_pub_wl_fake.return_val = false;
  ASSERT_EQ(bm_service_request(strlen(service), service, size,
                               (const uint8_t *)data, service_handler, 1000),
            false);
  bm_sub_wl_fake.return_val = false;
  ASSERT_EQ(bm_service_request(strlen(service), service, size,
                               (const uint8_t *)data, service_handler, 1000),
            false);
  free(data);
}
