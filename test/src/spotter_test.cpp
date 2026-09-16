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
  bm_pub_fake.return_val = BmOK;
  ASSERT_EQ(spotter_log(node_id, file_name, print_time, "%s:%s", s1, s2), BmOK);
  ASSERT_EQ(spotter_log(node_id, NULL, print_time, "%s:%s", s1, s2), BmOK);

  // Test improper use cases
  ASSERT_EQ(spotter_log(node_id, file_name, print_time, ""), BmENODATA);
  ASSERT_EQ(spotter_log(node_id, long_file_name, print_time, "%s:%s", s1, s2),
            BmEMSGSIZE);
  ASSERT_EQ(spotter_log(node_id, file_name, print_time, "%s", long_string),
            BmEMSGSIZE);
  bm_pub_fake.return_val = BmEBADMSG;
  ASSERT_EQ(spotter_log(node_id, file_name, print_time, "%s:%s", s1, s2),
            BmENETDOWN);
  ASSERT_EQ(spotter_log(node_id, NULL, print_time, "%s:%s", s1, s2),
            BmENETDOWN);
}

TEST_F(Spotter, tx_data) {
  uint8_t buf[1000];
  RND.rnd_array(buf, sizeof(buf));

  // Max message sizes
  bm_pub_fake.return_val = BmOK;
  ASSERT_EQ(spotter_tx_data(buf, 311, BmNetworkTypeCellularIriFallback), BmOK);
  ASSERT_EQ(spotter_tx_data(buf, 1000, BmNetworkTypeCellularOnly), BmOK);

  // Zero length data succeeds
  ASSERT_EQ(spotter_tx_data(buf, 0, BmNetworkTypeCellularIriFallback), BmOK);
  ASSERT_EQ(spotter_tx_data(buf, 0, BmNetworkTypeCellularOnly), BmOK);

  // Payload too big
  ASSERT_EQ(spotter_tx_data(buf, 312, BmNetworkTypeCellularIriFallback),
            BmEMSGSIZE);
  ASSERT_EQ(spotter_tx_data(buf, 1001, BmNetworkTypeCellularOnly), BmEMSGSIZE);

  // bm_pub error should pass through
  bm_pub_fake.return_val = BmEBADMSG;
  ASSERT_EQ(spotter_tx_data(buf, 100, BmNetworkTypeCellularIriFallback),
            BmEBADMSG);
  ASSERT_EQ(spotter_tx_data(buf, 100, BmNetworkTypeCellularOnly), BmEBADMSG);
  bm_pub_fake.return_val = BmENOMEM;
  ASSERT_EQ(spotter_tx_data(buf, 100, BmNetworkTypeCellularIriFallback),
            BmENOMEM);
  ASSERT_EQ(spotter_tx_data(buf, 100, BmNetworkTypeCellularOnly), BmENOMEM);
}

// Capture of the last buffer handed to bm_pub, so we can inspect the bytes
// that actually go out on the wire before spotter_tx_data frees them.
static uint8_t LAST_PUB_BUF[64];
static uint16_t LAST_PUB_LEN;

static BmErr capture_pub(const char *topic, const void *data, uint16_t data_len,
                         uint8_t type, uint8_t version) {
  (void)topic;
  (void)type;
  (void)version;
  LAST_PUB_LEN = data_len;
  memcpy(LAST_PUB_BUF, data,
         data_len < sizeof(LAST_PUB_BUF) ? data_len : sizeof(LAST_PUB_BUF));
  return BmOK;
}

TEST_F(Spotter, tx_data_network_type_is_one_byte) {
  ASSERT_EQ(sizeof(BmSerialNetworkType), 1U);
  ASSERT_EQ(sizeof(BmSerialNetworkDataHeader), 1U);
  ASSERT_EQ(offsetof(BmSerialNetworkDataHeader, data), 1U);

  const uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF};
  LAST_PUB_LEN = 0;
  memset(LAST_PUB_BUF, 0, sizeof(LAST_PUB_BUF));
  bm_pub_fake.custom_fake = capture_pub;

  ASSERT_EQ(
      spotter_tx_data(payload, sizeof(payload), BmNetworkTypeCellularOnly),
      BmOK);

  bm_pub_fake.custom_fake = NULL;

  ASSERT_EQ(LAST_PUB_LEN, 1U + sizeof(payload));
  ASSERT_EQ(LAST_PUB_BUF[0], BmNetworkTypeCellularOnly);
  ASSERT_EQ(memcmp(&LAST_PUB_BUF[1], payload, sizeof(payload)), 0);
}
