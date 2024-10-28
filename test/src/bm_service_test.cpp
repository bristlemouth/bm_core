#include <gtest/gtest.h>
#include <helpers.hpp>
#include <string.h>

#include "fff.h"

DEFINE_FFF_GLOBALS;

extern "C" {
#include "bm_service.h"
#include "mock_bm_os.h"
#include "mock_pubsub.h"
}

class BmService : public ::testing::Test {
public:
private:
protected:
  const char *service = "/rand/service/name";
  BmService() {}
  ~BmService() override {}

  void SetUp() override {}
  void TearDown() override {}
  static bool service_handler(size_t service_strlen, const char *service,
                              size_t req_data_len, uint8_t *req_data,
                              size_t *buffer_len, uint8_t *reply_data) {
    (void)service_strlen;
    (void)service;
    (void)req_data_len;
    (void)req_data_len;
    (void)buffer_len;
    (void)reply_data;
    return true;
  }
};

TEST_F(BmService, init) { bm_service_init(); }

TEST_F(BmService, register) {
  bm_semaphore_take_fake.return_val = BmOK;
  bm_sub_wl_fake.return_val = true;
  ASSERT_EQ(bm_service_register(strlen(service), service, service_handler),
            true);

  bm_sub_wl_fake.return_val = false;
  ASSERT_EQ(bm_service_register(strlen(service), service, service_handler),
            false);

  bm_semaphore_take_fake.return_val = BmETIMEDOUT;
  ASSERT_EQ(bm_service_register(strlen(service), service, service_handler),
            false);

  RESET_FAKE(bm_semaphore_take);
  RESET_FAKE(bm_sub_wl);
}

TEST_F(BmService, unregister) {
  // Add then remove service
  bm_semaphore_take_fake.return_val = BmOK;
  bm_sub_wl_fake.return_val = true;
  ASSERT_EQ(bm_service_register(strlen(service), service, service_handler),
            true);
  bm_unsub_wl_fake.return_val = true;
  ASSERT_EQ(bm_service_unregister(strlen(service), service), true);

  bm_semaphore_take_fake.return_val = BmOK;
  bm_sub_wl_fake.return_val = true;
  ASSERT_EQ(bm_service_register(strlen(service), service, service_handler),
            true);
  bm_unsub_wl_fake.return_val = false;
  ASSERT_EQ(bm_service_unregister(strlen(service), service), false);

  bm_semaphore_take_fake.return_val = BmOK;
  bm_sub_wl_fake.return_val = true;
  ASSERT_EQ(bm_service_register(strlen(service), service, service_handler),
            true);
  bm_semaphore_take_fake.return_val = BmETIMEDOUT;
  ASSERT_EQ(bm_service_unregister(strlen(service), service), false);

  RESET_FAKE(bm_semaphore_take);
  RESET_FAKE(bm_sub_wl);
  RESET_FAKE(bm_unsub_wl);
}
