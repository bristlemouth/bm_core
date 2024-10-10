#include <gtest/gtest.h>
#include <helpers.hpp>

#include "fff.h"

DEFINE_FFF_GLOBALS;

extern "C" {
#include "messages/info.h"
#include "mock_bcmp.h"
#include "mock_bm_os.h"
#include "mock_device.h"
#include "mock_neighbors.h"
#include "mock_packet.h"
}

static int CB_COUNT;

class info_test : public ::testing::Test {
public:
  rnd_gen RND;

private:
protected:
  info_test() {}
  ~info_test() override {}
  void SetUp() override { CB_COUNT = 0; }
  void TearDown() override {}
  static void request_info_cb_test(void *arg) {
    (void)arg;
    CB_COUNT++;
  };
};

TEST_F(info_test, request_info) {
  uint64_t target = (uint64_t)RND.rnd_int(UINT64_MAX, 1);
  uint32_t addr = (uint32_t)RND.rnd_int(UINT32_MAX, 0);
  BcmpProcessData data;
  BcmpDeviceInfoRequest request = {
      target,
  };
  BcmpDeviceInfoReply *reply;
  const char *device_name = "device name";
  const char *version_string = "version string";
  data.payload = (uint8_t *)&request;

  // Setup device return values
  node_id_fake.return_val = target;
  version_string_fake.return_val = version_string;
  device_name_fake.return_val = device_name;
  vendor_id_fake.return_val = (uint16_t)RND.rnd_int(UINT16_MAX, 0);
  product_id_fake.return_val = (uint16_t)RND.rnd_int(UINT16_MAX, 0);
  git_sha_fake.return_val = (uint32_t)RND.rnd_int(UINT32_MAX, 0);
  hardware_revision_fake.return_val = (uint8_t)RND.rnd_int(UINT8_MAX, 0);
  BcmpNeighbor neighbor = {0};

  reply = (BcmpDeviceInfoReply *)bm_malloc(sizeof(BcmpDeviceInfoReply) +
                                           strlen(device_name) +
                                           strlen(version_string));
  reply->info.node_id = target;
  reply->info.vendor_id = vendor_id_fake.return_val;
  reply->info.product_id = product_id_fake.return_val;
  reply->info.git_sha = git_sha_fake.return_val;
  reply->info.ver_major = (uint8_t)RND.rnd_int(UINT8_MAX, 0);
  reply->info.ver_minor = (uint8_t)RND.rnd_int(UINT8_MAX, 0);
  reply->info.ver_rev = (uint8_t)RND.rnd_int(UINT8_MAX, 0);
  reply->info.ver_hw = hardware_revision_fake.return_val;
  reply->ver_str_len = strlen(version_string);
  reply->dev_name_len = strlen(version_string);
  memcpy(&reply->strings[0], version_string, strlen(version_string));
  memcpy(&reply->strings[strlen(version_string)], device_name,
         strlen(device_name));

  // Initialize processing callbacks
  ASSERT_EQ(bcmp_device_info_init(), BmOK);

  // Test Bad Transmission
  bcmp_tx_fake.return_val = BmEBADMSG;
  ASSERT_EQ(bcmp_request_info(target, &addr, NULL), BmEBADMSG);
  RESET_FAKE(bcmp_tx);

  // Test Proper Transmission
  bcmp_tx_fake.return_val = BmOK;
  ASSERT_EQ(bcmp_request_info(target, &addr, NULL), BmOK);
  RESET_FAKE(bcmp_tx);

  // Test processing a request message
  serial_number_fake.return_val = BmOK;
  firmware_version_fake.return_val = BmOK;
  bcmp_tx_fake.return_val = BmOK;
  ASSERT_EQ(packet_process_invoke(BcmpDeviceInfoRequestMessage, data), BmOK);
  RESET_FAKE(bcmp_tx);

  // Test processing a request message to wrong Node
  node_id_fake.return_val = 0;
  ASSERT_EQ(packet_process_invoke(BcmpDeviceInfoRequestMessage, data), BmOK);
  RESET_FAKE(bcmp_tx);
  node_id_fake.return_val = target;

  // Test processing request w/ Null version/device string
  version_string_fake.return_val = NULL;
  device_name_fake.return_val = NULL;
  ASSERT_EQ(packet_process_invoke(BcmpDeviceInfoRequestMessage, data), BmOK);
  RESET_FAKE(bcmp_tx);

  // Test processing request w/ serial_number failure
  serial_number_fake.return_val = BmEINVAL;
  ASSERT_EQ(packet_process_invoke(BcmpDeviceInfoRequestMessage, data),
            BmEINVAL);

  // Test processing request w/ firmware_version failure
  serial_number_fake.return_val = BmOK;
  firmware_version_fake.return_val = BmEINVAL;
  ASSERT_EQ(packet_process_invoke(BcmpDeviceInfoRequestMessage, data),
            BmEINVAL);

  // Test processing request w/ tx failure
  serial_number_fake.return_val = BmOK;
  firmware_version_fake.return_val = BmOK;
  bcmp_tx_fake.return_val = BmEBADMSG;
  ASSERT_EQ(packet_process_invoke(BcmpDeviceInfoRequestMessage, data),
            BmEBADMSG);
  RESET_FAKE(bcmp_tx);

  data.payload = (uint8_t *)reply;
  // Test processing a reply
  bcmp_find_neighbor_fake.return_val = &neighbor;
  ASSERT_EQ(packet_process_invoke(BcmpDeviceInfoReplyMessage, data), BmOK);

  // Test no item in LL from expected reply
  ASSERT_NE(packet_process_invoke(BcmpDeviceInfoReplyMessage, data), BmOK);

  // Test expected node ID that is not a neighbor
  bcmp_expect_info_from_node_id(target);
  bcmp_tx_fake.return_val = BmOK;
  ASSERT_EQ(bcmp_request_info(target, &addr, NULL), BmOK);
  RESET_FAKE(bcmp_tx);
  bcmp_find_neighbor_fake.return_val = NULL;
  ASSERT_EQ(packet_process_invoke(BcmpDeviceInfoReplyMessage, data), BmOK);

  // Test No Strings
  bcmp_expect_info_from_node_id(target);
  bcmp_tx_fake.return_val = BmOK;
  reply->ver_str_len = 0;
  reply->dev_name_len = 0;
  ASSERT_EQ(bcmp_request_info(target, &addr, NULL), BmOK);
  RESET_FAKE(bcmp_tx);
  bcmp_find_neighbor_fake.return_val = NULL;
  ASSERT_EQ(packet_process_invoke(BcmpDeviceInfoReplyMessage, data), BmOK);

  // Test with callback
  bcmp_expect_info_from_node_id(target);
  bcmp_tx_fake.return_val = BmOK;
  ASSERT_EQ(bcmp_request_info(target, &addr, request_info_cb_test), BmOK);
  RESET_FAKE(bcmp_tx);
  bcmp_find_neighbor_fake.return_val = NULL;
  ASSERT_EQ(packet_process_invoke(BcmpDeviceInfoReplyMessage, data), BmOK);
  ASSERT_EQ(CB_COUNT, 1);

  // Test processing a request message to global Node
  request.target_node_id = 0;
  data.payload = (uint8_t *)&request;
  ASSERT_EQ(packet_process_invoke(BcmpDeviceInfoRequestMessage, data), BmOK);
  RESET_FAKE(bcmp_tx);

  packet_cleanup();
  bm_free(reply);
}
