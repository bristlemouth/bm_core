#include <gtest/gtest.h>
#include <helpers.hpp>

#include "fff.h"

DEFINE_FFF_GLOBALS;

#include "bcmp.h"
#include "mock_pubsub.h"
#include "spotter.h"

class Spotter : public ::testing::Test {
public:
  rnd_gen RND;

private:
protected:
  Spotter() {}
  ~Spotter() override {}
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(Spotter, printf) {
  uint64_t node_id = RND.rnd_int(UINT64_MAX, UINT8_MAX);
  const char *file_name = "hello_world.txt";
  char long_file_name[UINT8_MAX] = {0};
  char long_string[max_payload_len] = {0};
  uint8_t print_time = USE_TIMESTAMP;
  RND.rnd_str(long_file_name, UINT8_MAX);
  RND.rnd_str(long_string, max_payload_len);
  const char *s1 = "testing 1";
  const char *s2 = "testing 2";

  // Test proper use cases
  bm_pub_fake.return_val = true;
  ASSERT_EQ(spotter_log(node_id, file_name, print_time, "%s:%s", s1, s2), BmOK);
  ASSERT_EQ(spotter_log(node_id, NULL, print_time, "%s:%s", s1, s2), BmOK);

  // Test improper use cases
  ASSERT_EQ(spotter_log(node_id, file_name, print_time, ""), BmENODATA);
  ASSERT_EQ(spotter_log(node_id, long_file_name, print_time, "%s:%s", s1, s2),
            BmEMSGSIZE);
  ASSERT_EQ(spotter_log(node_id, file_name, print_time, "%s", long_string),
            BmEMSGSIZE);
  bm_pub_fake.return_val = false;
  ASSERT_EQ(spotter_log(node_id, file_name, print_time, "%s:%s", s1, s2),
            BmENETDOWN);
  ASSERT_EQ(spotter_log(node_id, NULL, print_time, "%s:%s", s1, s2),
            BmENETDOWN);
}

TEST_F(Spotter, tx_data) {
  uint8_t buf[UINT8_MAX];
  RND.rnd_array(buf, UINT8_MAX);

  // Test proper use cases
  bm_pub_fake.return_val = true;
  ASSERT_EQ(
      spotter_tx_data(buf, array_size(buf), BmNetworkTypeCellularIriFallback),
      BmOK);

  // Test improper use cases
  bm_pub_fake.return_val = false;
  ASSERT_EQ(
      spotter_tx_data(buf, array_size(buf), BmNetworkTypeCellularIriFallback),
      BmENETDOWN);
  ASSERT_EQ(spotter_tx_data(buf, UINT16_MAX, BmNetworkTypeCellularIriFallback),
            BmEINVAL);
}
