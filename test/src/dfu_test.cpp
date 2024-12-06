#include "gtest/gtest.h"

#include "fff.h"
extern "C" {
#include "dfu.h"
#include "mock_bcmp.h"
#include "mock_bm_dfu_generic.h"
#include "mock_bm_os.h"
#include "mock_configuration.h"
#include "mock_device.h"
#include "mock_packet.h"
#include "mock_timer_callback_handler.h"
}

DEFINE_FFF_GLOBALS;

using namespace testing;

// The fixture for testing class Foo.
class BcmpDfu : public ::testing::Test {
protected:
  // You can remove any or all of the following functions if its body
  // is empty.

  BcmpDfu() {
    // You can do set-up work for each test here.
  }

  ~BcmpDfu() override {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).
    RESET_FAKE(bm_queue_send);
    RESET_FAKE(bm_queue_create);
    RESET_FAKE(bm_task_create);
    RESET_FAKE(bm_timer_create);
    RESET_FAKE(node_id);
    RESET_FAKE(bcmp_tx);
    RESET_FAKE(bm_dfu_client_flash_area_open);
    RESET_FAKE(bm_dfu_client_flash_area_close);
    RESET_FAKE(bm_dfu_client_flash_area_write);
    RESET_FAKE(bm_dfu_client_flash_area_erase);
    RESET_FAKE(bm_dfu_client_flash_area_get_size);
    RESET_FAKE(bm_dfu_client_set_confirmed);
    RESET_FAKE(bm_dfu_host_get_chunk);
    RESET_FAKE(bm_dfu_core_lpm_peripheral_active);
    RESET_FAKE(bm_dfu_core_lpm_peripheral_inactive);
    RESET_FAKE(firmware_version);
    RESET_FAKE(git_sha);
    RESET_FAKE(get_config_uint);
    RESET_FAKE(set_config_uint);
    RESET_FAKE(save_config);
    fake_q = (BmQueue)malloc(sizeof(BmQueue));
    fake_timer = (BmTimer)malloc(sizeof(BmTimer));
    bm_queue_send_fake.return_val = BmOK;
    bm_task_create_fake.return_val = BmOK;
    bm_queue_create_fake.return_val = fake_q;
    bm_timer_create_fake.return_val = fake_timer;
    bm_dfu_client_flash_area_open_fake.return_val = BmOK;
    bm_dfu_client_flash_area_get_size_fake.return_val = 4096;
    bm_dfu_client_flash_area_erase_fake.return_val = BmOK;
    bm_dfu_client_flash_area_write_fake.return_val = BmOK;
    bm_dfu_client_flash_area_close_fake.return_val = BmOK;
    bm_dfu_client_flash_area_open_fake.return_val = BmOK;
    bm_dfu_host_get_chunk_fake.return_val = BmOK;
    node_id_fake.return_val = 0xdeadbeefbeeffeed;
    bcmp_tx_fake.return_val = BmOK;
    git_sha_fake.return_val = 0xd00dd00d;
    memset(&client_update_reboot_info, 0, sizeof(client_update_reboot_info));
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).
    free(fake_q);
  }

  // Objects declared here can be used by all tests in the test suite for Foo.
  BmQueue fake_q;
  BmTimer fake_timer;
  static constexpr size_t CHUNK_SIZE = 512;
  static constexpr size_t IMAGE_SIZE = 2048;
};

TEST_F(BcmpDfu, init_test) {
  bm_dfu_init();
  EXPECT_EQ(bm_queue_send_fake.call_count, 1);
  EXPECT_EQ(bm_task_create_fake.call_count, 1);
  EXPECT_EQ(bm_queue_create_fake.call_count, 1);
  EXPECT_EQ(bm_timer_create_fake.call_count, 4);
  EXPECT_EQ(node_id_fake.call_count, 3);
}

TEST_F(BcmpDfu, process_message_test) {
  bm_dfu_init();
  EXPECT_EQ(bm_queue_send_fake.call_count, 1);
  EXPECT_EQ(bm_task_create_fake.call_count, 1);
  EXPECT_EQ(bm_queue_create_fake.call_count, 1);
  EXPECT_EQ(bm_timer_create_fake.call_count, 4);
  EXPECT_EQ(node_id_fake.call_count, 3);

  // dfu start
  BcmpDfuStart *start_msg = (BcmpDfuStart *)malloc(sizeof(BcmpDfuStart));
  start_msg->header.frame_type = BcmpDFUStartMessage;
  start_msg->info.addresses.dst_node_id = 0xdeadbeefbeeffeed;
  start_msg->info.addresses.src_node_id = 0xdeaddeaddeaddead;
  bm_dfu_process_message((uint8_t *)start_msg, sizeof(BcmpDfuStart));
  EXPECT_EQ(bm_queue_send_fake.call_count, 2);
  free(start_msg);

  // payload req
  BcmpDfuPayloadReq *payload_req_msg =
      (BcmpDfuPayloadReq *)malloc(sizeof(BcmpDfuPayloadReq));
  payload_req_msg->header.frame_type = BcmpDFUPayloadReqMessage;
  payload_req_msg->chunk_req.addresses.dst_node_id = 0xdeadbeefbeeffeed;
  payload_req_msg->chunk_req.addresses.src_node_id = 0xdeaddeaddeaddead;
  bm_dfu_process_message((uint8_t *)payload_req_msg, sizeof(BcmpDfuPayloadReq));
  EXPECT_EQ(bm_queue_send_fake.call_count, 3);
  free(payload_req_msg);

  // payload
  BcmpDfuPayload *payload_msg =
      (BcmpDfuPayload *)malloc(sizeof(BcmpDfuPayload));
  payload_msg->header.frame_type = BcmpDFUPayloadMessage;
  payload_msg->chunk.addresses.dst_node_id = 0xdeadbeefbeeffeed;
  payload_msg->chunk.addresses.src_node_id = 0xdeaddeaddeaddead;
  bm_dfu_process_message((uint8_t *)payload_msg, sizeof(BcmpDfuPayload));
  EXPECT_EQ(bm_queue_send_fake.call_count, 4);
  free(payload_msg);

  // dfu end
  BcmpDfuEnd *end_msg = (BcmpDfuEnd *)malloc(sizeof(BcmpDfuEnd));
  end_msg->header.frame_type = BcmpDFUEndMessage;
  end_msg->result.addresses.dst_node_id = 0xdeadbeefbeeffeed;
  end_msg->result.addresses.src_node_id = 0xdeaddeaddeaddead;
  bm_dfu_process_message((uint8_t *)end_msg, sizeof(BcmpDfuEnd));
  EXPECT_EQ(bm_queue_send_fake.call_count, 5);
  free(end_msg);

  // dfu ack
  BcmpDfuAck *ack_msg = (BcmpDfuAck *)malloc(sizeof(BcmpDfuAck));
  ack_msg->header.frame_type = BcmpDFUAckMessage;
  ack_msg->ack.addresses.dst_node_id = 0xdeadbeefbeeffeed;
  ack_msg->ack.addresses.src_node_id = 0xdeaddeaddeaddead;
  bm_dfu_process_message((uint8_t *)ack_msg, sizeof(BcmpDfuAck));
  EXPECT_EQ(bm_queue_send_fake.call_count, 6);
  free(ack_msg);

  // dfu abort
  BcmpDfuAbort *abort_msg = (BcmpDfuAbort *)malloc(sizeof(BcmpDfuAbort));
  abort_msg->header.frame_type = BcmpDFUAbortMessage;
  abort_msg->err.addresses.dst_node_id = 0xdeadbeefbeeffeed;
  abort_msg->err.addresses.src_node_id = 0xdeaddeaddeaddead;
  bm_dfu_process_message((uint8_t *)abort_msg, sizeof(BcmpDfuAbort));
  EXPECT_EQ(bm_queue_send_fake.call_count, 7);
  free(abort_msg);

  // dfu heartbeat
  BcmpDfuHeartbeat *hb_msg =
      (BcmpDfuHeartbeat *)malloc(sizeof(BcmpDfuHeartbeat));
  hb_msg->header.frame_type = BcmpDFUHeartbeatMessage;
  hb_msg->addr.dst_node_id = 0xdeadbeefbeeffeed;
  hb_msg->addr.src_node_id = 0xdeaddeaddeaddead;
  bm_dfu_process_message((uint8_t *)hb_msg, sizeof(BcmpDfuHeartbeat));
  EXPECT_EQ(bm_queue_send_fake.call_count, 8);
  free(hb_msg);

  // TODO - This is another test that expects and assert, so refactor once we have removed the asserts and
  // return an actual error code
  // Invalid msg
  // BcmpDfuHeartbeat* bad_msg = (BcmpDfuHeartbeat*) malloc (sizeof(BcmpDfuHeartbeat));
  // bad_msg->header.frame_type = BcmpConfigCommitMessage;
  // bad_msg->addr.dst_node_id = 0xdeadbeefbeeffeed;
  // bad_msg->addr.src_node_id = 0xdeaddeaddeaddead;
  // EXPECT_DEATH(bm_dfu_process_message((uint8_t*)bad_msg, sizeof(BcmpDfuHeartbeat)),"");
  // free(bad_msg);
}

TEST_F(BcmpDfu, dfu_api_test) {
  bm_dfu_init();
  EXPECT_EQ(bm_queue_send_fake.call_count, 1);
  EXPECT_EQ(bm_task_create_fake.call_count, 1);
  EXPECT_EQ(bm_queue_create_fake.call_count, 1);
  EXPECT_EQ(bm_timer_create_fake.call_count, 4);
  EXPECT_EQ(node_id_fake.call_count, 3);

  bm_dfu_send_ack(0xdeadbeefbeeffeed, 1, BmDfuErrNone);
  EXPECT_EQ(bcmp_tx_fake.call_count, 1);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUAckMessage);

  bm_dfu_req_next_chunk(0xdeadbeefbeeffeed, 0);
  EXPECT_EQ(bcmp_tx_fake.call_count, 2);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUPayloadReqMessage);

  bm_dfu_update_end(0xdeadbeefbeeffeed, 1, BmDfuErrNone);
  EXPECT_EQ(bcmp_tx_fake.call_count, 3);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUEndMessage);

  bm_dfu_send_heartbeat(0xdeadbeefbeeffeed);
  EXPECT_EQ(bcmp_tx_fake.call_count, 4);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUHeartbeatMessage);

  BmDfuImgInfo info;
  info.chunk_size = bm_dfu_max_chunk_size;
  info.crc16 = 0xbaad;
  info.image_size = 2 * 1000 * 1024;
  info.major_ver = 0;
  info.minor_ver = 1;
  info.gitSHA = 0xd00dd00d;
  EXPECT_EQ(bm_dfu_initiate_update(info, 0xdeadbeefbeeffeed, NULL, 1000),
            false);
}

TEST_F(BcmpDfu, client_golden) {
  git_sha_fake.return_val = 0xbaaddaad;

  // INIT SUCCESS
  bm_dfu_init();
  LibSmContext *ctx = bm_dfu_test_get_sm_ctx();
  BmDfuEvent evt = {
      .type = DfuEventInitSuccess,
      .buf = NULL,
      .len = 0,
  };
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateIdle);

  // DFU REQUEST
  evt.type = DfuEventReceivedUpdateRequest;
  evt.buf = (uint8_t *)malloc(sizeof(BcmpDfuStart));
  evt.len = sizeof(BcmpDfuStart);
  BcmpDfuStart dfu_start_msg;
  dfu_start_msg.header.frame_type = BcmpDFUStartMessage;
  dfu_start_msg.info.addresses.src_node_id = 0xbeefbeefdaadbaad;
  dfu_start_msg.info.addresses.dst_node_id = 0xdeadbeefbeeffeed;
  dfu_start_msg.info.img_info.image_size = IMAGE_SIZE;
  dfu_start_msg.info.img_info.chunk_size = CHUNK_SIZE;
  dfu_start_msg.info.img_info.crc16 = 0x2fDf;
  dfu_start_msg.info.img_info.major_ver = 1;
  dfu_start_msg.info.img_info.minor_ver = 7;
  dfu_start_msg.info.img_info.gitSHA = 0xdeadd00d;
  memcpy(evt.buf, &dfu_start_msg, sizeof(BcmpDfuStart));

  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(bcmp_tx_fake.arg1_history[0], BcmpDFUAckMessage);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUPayloadReqMessage);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientReceiving);

  // Chunk
  evt.type = DfuEventImageChunk;
  evt.buf = (uint8_t *)malloc(sizeof(BcmpDfuPayload) + CHUNK_SIZE);
  evt.len = sizeof(BcmpDfuPayload) + CHUNK_SIZE;
  BcmpDfuPayload dfu_payload_msg;
  dfu_payload_msg.header.frame_type = BcmpDFUPayloadMessage;
  dfu_payload_msg.chunk.addresses.src_node_id = 0xbeefbeefdaadbaad;
  dfu_payload_msg.chunk.addresses.dst_node_id = 0xdeadbeefbeeffeed;
  dfu_payload_msg.chunk.payload_length = CHUNK_SIZE;
  memcpy(evt.buf, &dfu_payload_msg, sizeof(dfu_payload_msg));
  memset(evt.buf + sizeof(dfu_payload_msg), 0xa5, CHUNK_SIZE);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // 512
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUPayloadReqMessage);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientReceiving);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // 1024
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUPayloadReqMessage);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientReceiving);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // 1536
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUPayloadReqMessage);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientReceiving);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // 2048
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientValidating);

  // Validating
  evt.type = DfuEventNone;
  evt.buf = NULL;
  evt.len = 0;
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientRebootReq);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFURebootReqMessage);

  // Reboot
  evt.type = DfuEventReboot;
  evt.buf = (uint8_t *)malloc(sizeof(BcmpDfuReboot));
  evt.len = sizeof(BcmpDfuReboot);
  BcmpDfuReboot dfu_reboot_msg;
  dfu_reboot_msg.header.frame_type = BcmpDFURebootMessage;
  dfu_reboot_msg.addr.src_node_id = 0xbeefbeefdaadbaad;
  dfu_reboot_msg.addr.dst_node_id = 0xdeadbeefbeeffeed;
  memcpy(evt.buf, &dfu_reboot_msg, sizeof(dfu_reboot_msg));
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx),
            BmDfuStateClientActivating); // We reboot in this step.
  // See ClientImageHasUpdated for state behavior after reboot.
}

TEST_F(BcmpDfu, client_reject_same_sha) {
  git_sha_fake.return_val = 0xdeadd00d; // same SHA

  // INIT SUCCESS
  bm_dfu_init();
  LibSmContext *ctx = bm_dfu_test_get_sm_ctx();
  BmDfuEvent evt = {
      .type = DfuEventInitSuccess,
      .buf = NULL,
      .len = 0,
  };
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateIdle);

  // DFU REQUEST
  evt.type = DfuEventReceivedUpdateRequest;
  evt.buf = (uint8_t *)malloc(sizeof(BcmpDfuStart));
  evt.len = sizeof(BcmpDfuStart);
  BcmpDfuStart dfu_start_msg;
  dfu_start_msg.header.frame_type = BcmpDFUStartMessage;
  dfu_start_msg.info.addresses.src_node_id = 0xbeefbeefdaadbaad;
  dfu_start_msg.info.addresses.dst_node_id = 0xdeadbeefbeeffeed;
  dfu_start_msg.info.img_info.image_size = IMAGE_SIZE;
  dfu_start_msg.info.img_info.chunk_size = CHUNK_SIZE;
  dfu_start_msg.info.img_info.crc16 = 0x2fDf;
  dfu_start_msg.info.img_info.major_ver = 1;
  dfu_start_msg.info.img_info.minor_ver = 7;
  dfu_start_msg.info.img_info.gitSHA = 0xdeadd00d;
  memcpy(evt.buf, &dfu_start_msg, sizeof(BcmpDfuStart));

  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUAckMessage);
  EXPECT_EQ(get_current_state_enum(ctx),
            BmDfuStateIdle); // We don't progress to RECEIVING.
}

TEST_F(BcmpDfu, client_force_update) {
  git_sha_fake.return_val = 0xdeadd00d; // same SHA

  // INIT SUCCESS
  bm_dfu_init();
  LibSmContext *ctx = bm_dfu_test_get_sm_ctx();
  BmDfuEvent evt = {
      .type = DfuEventInitSuccess,
      .buf = NULL,
      .len = 0,
  };
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateIdle);

  // DFU REQUEST
  evt.type = DfuEventReceivedUpdateRequest;
  evt.buf = (uint8_t *)malloc(sizeof(BcmpDfuStart));
  evt.len = sizeof(BcmpDfuStart);
  BcmpDfuStart dfu_start_msg;
  dfu_start_msg.header.frame_type = BcmpDFUStartMessage;
  dfu_start_msg.info.addresses.src_node_id = 0xbeefbeefdaadbaad;
  dfu_start_msg.info.addresses.dst_node_id = 0xdeadbeefbeeffeed;
  dfu_start_msg.info.img_info.image_size = IMAGE_SIZE;
  dfu_start_msg.info.img_info.chunk_size = CHUNK_SIZE;
  dfu_start_msg.info.img_info.crc16 = 0x2fDf;
  dfu_start_msg.info.img_info.major_ver = 1;
  dfu_start_msg.info.img_info.minor_ver = 7;
  dfu_start_msg.info.img_info.gitSHA = 0xdeadd00d;
  dfu_start_msg.info.img_info.filter_key =
      BM_DFU_IMG_INFO_FORCE_UPDATE; // forced update
  memcpy(evt.buf, &dfu_start_msg, sizeof(BcmpDfuStart));

  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(bcmp_tx_fake.arg1_history[0], BcmpDFUAckMessage);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUPayloadReqMessage);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientReceiving);
}

TEST_F(BcmpDfu, client_golden_image_has_updated) {
  // Set the reboot info.
  client_update_reboot_info.magic = DFU_REBOOT_MAGIC;
  client_update_reboot_info.host_node_id = 0xbeefbeefdaadbaad;
  client_update_reboot_info.major = 1;
  client_update_reboot_info.minor = 7;
  client_update_reboot_info.gitSHA = 0xdeadd00d;
  git_sha_fake.return_val = 0xdeadd00d;

  // INIT SUCCESS
  bm_dfu_init();
  LibSmContext *ctx = bm_dfu_test_get_sm_ctx();
  BmDfuEvent evt = {
      .type = DfuEventInitSuccess,
      .buf = NULL,
      .len = 0,
  };
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientRebootDone);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUBootCompleteMessage);

  // REBOOT_DONE
  evt.type = DfuEventUpdateEnd;
  evt.buf = (uint8_t *)malloc(sizeof(BcmpDfuEnd));
  evt.len = sizeof(BcmpDfuEnd);
  BcmpDfuEnd bcmp_end_msg;
  bcmp_end_msg.header.frame_type = BcmpDFUEndMessage;
  bcmp_end_msg.result.addresses.dst_node_id = 0xdeadbeefbeeffeed;
  bcmp_end_msg.result.addresses.src_node_id = 0xbeefbeefdaadbaad;
  bcmp_end_msg.result.err_code = BmDfuErrNone;
  bcmp_end_msg.result.success = 1;
  memcpy(evt.buf, &bcmp_end_msg, sizeof(bcmp_end_msg));
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateIdle);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUEndMessage);
}

TEST_F(BcmpDfu, client_resync_host) {
  // INIT SUCCESS
  bm_dfu_init();
  LibSmContext *ctx = bm_dfu_test_get_sm_ctx();
  BmDfuEvent evt = {
      .type = DfuEventInitSuccess,
      .buf = NULL,
      .len = 0,
  };
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateIdle);

  // DFU REQUEST
  evt.type = DfuEventReceivedUpdateRequest;
  evt.buf = (uint8_t *)malloc(sizeof(BcmpDfuStart));
  evt.len = sizeof(BcmpDfuStart);
  BcmpDfuStart dfu_start_msg;
  dfu_start_msg.header.frame_type = BcmpDFUStartMessage;
  dfu_start_msg.info.addresses.src_node_id = 0xbeefbeefdaadbaad;
  dfu_start_msg.info.addresses.dst_node_id = 0xdeadbeefbeeffeed;
  dfu_start_msg.info.img_info.image_size = IMAGE_SIZE;
  dfu_start_msg.info.img_info.chunk_size = CHUNK_SIZE;
  dfu_start_msg.info.img_info.crc16 = 0x2fDf;
  dfu_start_msg.info.img_info.major_ver = 1;
  dfu_start_msg.info.img_info.minor_ver = 7;
  dfu_start_msg.info.img_info.gitSHA = 0xdeadd00d;
  memcpy(evt.buf, &dfu_start_msg, sizeof(BcmpDfuStart));

  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(bcmp_tx_fake.arg1_history[0], BcmpDFUAckMessage);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUPayloadReqMessage);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientReceiving);

  bm_dfu_test_set_dfu_event_and_run_sm(
      evt); // Get a DfuEventReceivedUpdateRequest in BmDfuStateClientReceiving state
  EXPECT_EQ(bcmp_tx_fake.arg1_history[0], BcmpDFUAckMessage);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUPayloadReqMessage);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientReceiving);
}

TEST_F(BcmpDfu, host_golden) {
  // INIT SUCCESS
  bm_dfu_init();
  LibSmContext *ctx = bm_dfu_test_get_sm_ctx();
  BmDfuEvent evt = {
      .type = DfuEventInitSuccess,
      .buf = NULL,
      .len = 0,
  };
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateIdle);

  // HOST REQUEST
  evt.type = DfuEventBeginHost;
  evt.buf = (uint8_t *)malloc(sizeof(DfuHostStartEvent));
  evt.len = sizeof(DfuHostStartEvent);
  DfuHostStartEvent dfu_start_msg;
  dfu_start_msg.start.header.frame_type = BcmpDFUStartMessage;
  dfu_start_msg.start.info.addresses.src_node_id = 0xdeadbeefbeeffeed;
  dfu_start_msg.start.info.addresses.dst_node_id = 0xbeefbeefdaadbaad;
  dfu_start_msg.start.info.img_info.image_size = IMAGE_SIZE;
  dfu_start_msg.start.info.img_info.chunk_size = CHUNK_SIZE;
  dfu_start_msg.start.info.img_info.crc16 = 0x2fDf;
  dfu_start_msg.start.info.img_info.major_ver = 1;
  dfu_start_msg.start.info.img_info.minor_ver = 7;
  dfu_start_msg.timeoutMs = 30000;
  dfu_start_msg.finish_cb = NULL;
  dfu_start_msg.start.info.img_info.gitSHA = 0xdeadd00d;
  memcpy(evt.buf, &dfu_start_msg, sizeof(dfu_start_msg));

  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateHostReqUpdate);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUStartMessage);

  // HOST UPDATE
  evt.type = DfuEventAckReceived;
  evt.buf = (uint8_t *)malloc(sizeof(BcmpDfuAck));
  evt.len = sizeof(BcmpDfuAck);
  BcmpDfuAck dfu_ack_msg;
  dfu_ack_msg.header.frame_type = BcmpDFUAckMessage;
  dfu_ack_msg.ack.addresses.dst_node_id = 0xdeadbeefbeeffeed;
  dfu_ack_msg.ack.addresses.src_node_id = 0xbeefbeefdaadbaad;
  dfu_ack_msg.ack.err_code = BmDfuErrNone;
  dfu_ack_msg.ack.success = 1;
  memcpy(evt.buf, &dfu_ack_msg, sizeof(BcmpDfuAck));
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateHostUpdate);

  // CHUNK REQUEST
  evt.type = DfuEventChunkRequest;
  evt.buf = (uint8_t *)malloc(sizeof(BcmpDfuPayloadReq));
  evt.len = sizeof(BcmpDfuPayloadReq);
  BcmpDfuPayloadReq dfu_payload_req_msg;
  dfu_payload_req_msg.header.frame_type = BcmpDFUPayloadReqMessage;
  dfu_payload_req_msg.chunk_req.addresses.dst_node_id = 0xdeadbeefbeeffeed;
  dfu_payload_req_msg.chunk_req.addresses.src_node_id = 0xbeefbeefdaadbaad;
  dfu_payload_req_msg.chunk_req.seq_num = 0;
  memcpy(evt.buf, &dfu_payload_req_msg, sizeof(dfu_payload_req_msg));
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateHostUpdate);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUPayloadMessage);

  // REBOOT REQUEST
  evt.type = DfuEventRebootRequest;
  evt.buf = (uint8_t *)malloc(sizeof(BcmpDfuRebootReq));
  evt.len = sizeof(BcmpDfuRebootReq);
  BcmpDfuRebootReq dfu_reboot_req_msg;
  dfu_reboot_req_msg.header.frame_type = BcmpRebootRequestMessage;
  dfu_reboot_req_msg.addr.dst_node_id = 0xdeadbeefbeeffeed;
  dfu_reboot_req_msg.addr.src_node_id = 0xbeefbeefdaadbaad;
  memcpy(evt.buf, &dfu_reboot_req_msg, sizeof(dfu_reboot_req_msg));
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateHostUpdate);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFURebootMessage);

  // REBOOT COMPLETE
  evt.type = DfuEventBootComplete;
  evt.buf = (uint8_t *)malloc(sizeof(BcmpDfuBootComplete));
  evt.len = sizeof(BcmpDfuBootComplete);
  BcmpDfuBootComplete dfu_reboot_done_msg;
  dfu_reboot_done_msg.header.frame_type = BcmpRebootRequestMessage;
  dfu_reboot_done_msg.addr.dst_node_id = 0xdeadbeefbeeffeed;
  dfu_reboot_done_msg.addr.src_node_id = 0xbeefbeefdaadbaad;
  memcpy(evt.buf, &dfu_reboot_done_msg, sizeof(dfu_reboot_done_msg));
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateHostUpdate);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUEndMessage);

  // DFU EVENT
  evt.type = DfuEventUpdateEnd;
  evt.buf = (uint8_t *)malloc(sizeof(BcmpDfuEnd));
  evt.len = sizeof(BcmpDfuEnd);
  BcmpDfuEnd dfu_end_msg;
  dfu_end_msg.header.frame_type = BcmpDFUEndMessage;
  dfu_end_msg.result.addresses.dst_node_id = 0xdeadbeefbeeffeed;
  dfu_end_msg.result.addresses.src_node_id = 0xbeefbeefdaadbaad;
  dfu_end_msg.result.err_code = BmDfuErrNone;
  dfu_end_msg.result.success = 1;
  memcpy(evt.buf, &dfu_end_msg, sizeof(dfu_end_msg));
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateIdle);
}

TEST_F(BcmpDfu, host_req_update_fail) {
  // INIT SUCCESS
  bm_dfu_init();
  LibSmContext *ctx = bm_dfu_test_get_sm_ctx();
  BmDfuEvent evt = {
      .type = DfuEventInitSuccess,
      .buf = NULL,
      .len = 0,
  };
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateIdle);

  // HOST REQUEST
  evt.type = DfuEventBeginHost;
  evt.buf = (uint8_t *)malloc(sizeof(DfuHostStartEvent));
  evt.len = sizeof(DfuHostStartEvent);
  DfuHostStartEvent dfu_start_msg;
  dfu_start_msg.start.header.frame_type = BcmpDFUStartMessage;
  dfu_start_msg.start.info.addresses.src_node_id = 0xdeadbeefbeeffeed;
  dfu_start_msg.start.info.addresses.dst_node_id = 0xbeefbeefdaadbaad;
  dfu_start_msg.start.info.img_info.image_size = IMAGE_SIZE;
  dfu_start_msg.start.info.img_info.chunk_size = CHUNK_SIZE;
  dfu_start_msg.start.info.img_info.crc16 = 0x2fDf;
  dfu_start_msg.start.info.img_info.major_ver = 1;
  dfu_start_msg.start.info.img_info.minor_ver = 7;
  dfu_start_msg.timeoutMs = 30000;
  dfu_start_msg.finish_cb = NULL;
  dfu_start_msg.start.info.img_info.gitSHA = 0xdeadd00d;
  memcpy(evt.buf, &dfu_start_msg, sizeof(dfu_start_msg));

  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateHostReqUpdate);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUStartMessage);

  evt.type = DfuEventAckTimeout;
  evt.buf = NULL;
  evt.len = 0;
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateHostReqUpdate); // retry 1
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateError);
  evt.type = DfuEventNone;
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateIdle);

  // HOST REQUEST
  evt.type = DfuEventBeginHost;
  evt.buf = (uint8_t *)malloc(sizeof(DfuHostStartEvent));
  evt.len = sizeof(DfuHostStartEvent);
  memcpy(evt.buf, &dfu_start_msg, sizeof(DfuHostStartEvent));

  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateHostReqUpdate);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUStartMessage);

  // ABORT
  evt.type = DfuEventAbort;
  evt.buf = NULL;
  evt.len = 0;
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateError);
  evt.type = DfuEventNone;
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateIdle);
}

TEST_F(BcmpDfu, host_update_fail) {
  // INIT SUCCESS
  bm_dfu_init();
  LibSmContext *ctx = bm_dfu_test_get_sm_ctx();
  BmDfuEvent evt = {
      .type = DfuEventInitSuccess,
      .buf = NULL,
      .len = 0,
  };
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateIdle);

  // HOST REQUEST
  evt.type = DfuEventBeginHost;
  evt.buf = (uint8_t *)malloc(sizeof(DfuHostStartEvent));
  evt.len = sizeof(DfuHostStartEvent);
  DfuHostStartEvent dfu_start_msg;
  dfu_start_msg.start.header.frame_type = BcmpDFUStartMessage;
  dfu_start_msg.start.info.addresses.src_node_id = 0xdeadbeefbeeffeed;
  dfu_start_msg.start.info.addresses.dst_node_id = 0xbeefbeefdaadbaad;
  dfu_start_msg.start.info.img_info.image_size = IMAGE_SIZE;
  dfu_start_msg.start.info.img_info.chunk_size = CHUNK_SIZE;
  dfu_start_msg.start.info.img_info.crc16 = 0x2fDf;
  dfu_start_msg.start.info.img_info.major_ver = 1;
  dfu_start_msg.start.info.img_info.minor_ver = 7;
  dfu_start_msg.timeoutMs = 30000;
  dfu_start_msg.finish_cb = NULL;
  dfu_start_msg.start.info.img_info.gitSHA = 0xdeadd00d;
  memcpy(evt.buf, &dfu_start_msg, sizeof(dfu_start_msg));

  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateHostReqUpdate);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUStartMessage);

  // HOST UPDATE
  evt.type = DfuEventAckReceived;
  evt.buf = (uint8_t *)malloc(sizeof(BcmpDfuAck));
  evt.len = sizeof(BcmpDfuAck);
  BcmpDfuAck dfu_ack_msg;
  dfu_ack_msg.header.frame_type = BcmpDFUAckMessage;
  dfu_ack_msg.ack.addresses.dst_node_id = 0xdeadbeefbeeffeed;
  dfu_ack_msg.ack.addresses.src_node_id = 0xbeefbeefdaadbaad;
  dfu_ack_msg.ack.err_code = BmDfuErrNone;
  dfu_ack_msg.ack.success = 1;
  memcpy(evt.buf, &dfu_ack_msg, sizeof(BcmpDfuAck));
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateHostUpdate);

  // ABORT
  evt.type = DfuEventAbort;
  evt.buf = NULL;
  evt.len = 0;
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateError);
  evt.type = DfuEventNone;
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateIdle);

  // HOST REQUEST
  evt.type = DfuEventBeginHost;
  evt.buf = (uint8_t *)malloc(sizeof(DfuHostStartEvent));
  evt.len = sizeof(DfuHostStartEvent);
  memcpy(evt.buf, &dfu_start_msg, sizeof(dfu_start_msg));

  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateHostReqUpdate);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUStartMessage);

  // HOST UPDATE
  evt.type = DfuEventAckReceived;
  evt.buf = (uint8_t *)malloc(sizeof(BcmpDfuAck));
  evt.len = sizeof(BcmpDfuAck);
  memcpy(evt.buf, &dfu_ack_msg, sizeof(BcmpDfuAck));
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateHostUpdate);
}

TEST_F(BcmpDfu, host_update_fail_upon_reboot) {
  // INIT SUCCESS
  bm_dfu_init();
  LibSmContext *ctx = bm_dfu_test_get_sm_ctx();
  BmDfuEvent evt = {
      .type = DfuEventInitSuccess,
      .buf = NULL,
      .len = 0,
  };
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateIdle);

  // HOST REQUEST
  evt.type = DfuEventBeginHost;
  evt.buf = (uint8_t *)malloc(sizeof(DfuHostStartEvent));
  evt.len = sizeof(DfuHostStartEvent);
  DfuHostStartEvent dfu_start_msg;
  dfu_start_msg.start.header.frame_type = BcmpDFUStartMessage;
  dfu_start_msg.start.info.addresses.src_node_id = 0xdeadbeefbeeffeed;
  dfu_start_msg.start.info.addresses.dst_node_id = 0xbeefbeefdaadbaad;
  dfu_start_msg.start.info.img_info.image_size = IMAGE_SIZE;
  dfu_start_msg.start.info.img_info.chunk_size = CHUNK_SIZE;
  dfu_start_msg.start.info.img_info.crc16 = 0x2fDf;
  dfu_start_msg.start.info.img_info.major_ver = 1;
  dfu_start_msg.start.info.img_info.minor_ver = 7;
  dfu_start_msg.timeoutMs = 30000;
  dfu_start_msg.finish_cb = NULL;
  dfu_start_msg.start.info.img_info.gitSHA = 0xdeadd00d;
  memcpy(evt.buf, &dfu_start_msg, sizeof(dfu_start_msg));

  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateHostReqUpdate);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUStartMessage);

  // HOST UPDATE
  evt.type = DfuEventAckReceived;
  evt.buf = (uint8_t *)malloc(sizeof(BcmpDfuAck));
  evt.len = sizeof(BcmpDfuAck);
  BcmpDfuAck dfu_ack_msg;
  dfu_ack_msg.header.frame_type = BcmpDFUAckMessage;
  dfu_ack_msg.ack.addresses.dst_node_id = 0xdeadbeefbeeffeed;
  dfu_ack_msg.ack.addresses.src_node_id = 0xbeefbeefdaadbaad;
  dfu_ack_msg.ack.err_code = BmDfuErrNone;
  dfu_ack_msg.ack.success = 1;
  memcpy(evt.buf, &dfu_ack_msg, sizeof(BcmpDfuAck));
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateHostUpdate);

  // REBOOT REQUEST
  evt.type = DfuEventRebootRequest;
  evt.buf = (uint8_t *)malloc(sizeof(BcmpDfuRebootReq));
  evt.len = sizeof(BcmpDfuRebootReq);
  BcmpDfuRebootReq dfu_reboot_req_msg;
  dfu_reboot_req_msg.header.frame_type = BcmpRebootRequestMessage;
  dfu_reboot_req_msg.addr.dst_node_id = 0xdeadbeefbeeffeed;
  dfu_reboot_req_msg.addr.src_node_id = 0xbeefbeefdaadbaad;
  memcpy(evt.buf, &dfu_reboot_req_msg, sizeof(dfu_reboot_req_msg));
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateHostUpdate);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFURebootMessage);

  // REBOOT COMPLETE
  evt.type = DfuEventBootComplete;
  evt.buf = (uint8_t *)malloc(sizeof(BcmpDfuBootComplete));
  evt.len = sizeof(BcmpDfuBootComplete);
  BcmpDfuBootComplete dfu_reboot_done_msg;
  dfu_reboot_done_msg.header.frame_type = BcmpRebootRequestMessage;
  dfu_reboot_done_msg.addr.dst_node_id = 0xdeadbeefbeeffeed;
  dfu_reboot_done_msg.addr.src_node_id = 0xbeefbeefdaadbaad;
  memcpy(evt.buf, &dfu_reboot_done_msg, sizeof(dfu_reboot_done_msg));
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateHostUpdate);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUEndMessage);

  // ABORT REBOOT CHECK
  BcmpDfuAbort *abort = (BcmpDfuAbort *)malloc(sizeof(BcmpDfuAbort));
  abort->err.err_code = BmDfuErrConfirmationAbort;
  evt.type = DfuEventAbort;
  evt.buf = (uint8_t *)abort;
  evt.len = sizeof(BcmpDfuBootComplete);
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateError);
  EXPECT_EQ(bm_dfu_get_error(), BmDfuErrConfirmationAbort);
  evt.type = DfuEventNone;
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateIdle);
}

TEST_F(BcmpDfu, client_recv_fail) {
  // INIT SUCCESS
  bm_dfu_init();
  LibSmContext *ctx = bm_dfu_test_get_sm_ctx();
  BmDfuEvent evt = {
      .type = DfuEventInitSuccess,
      .buf = NULL,
      .len = 0,
  };
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateIdle);

  // DFU REQUEST
  evt.type = DfuEventReceivedUpdateRequest;
  evt.buf = (uint8_t *)malloc(sizeof(BcmpDfuStart));
  evt.len = sizeof(BcmpDfuStart);
  BcmpDfuStart dfu_start_msg;
  dfu_start_msg.header.frame_type = BcmpDFUStartMessage;
  dfu_start_msg.info.addresses.src_node_id = 0xbeefbeefdaadbaad;
  dfu_start_msg.info.addresses.dst_node_id = 0xdeadbeefbeeffeed;
  dfu_start_msg.info.img_info.image_size = IMAGE_SIZE;
  dfu_start_msg.info.img_info.chunk_size = CHUNK_SIZE;
  dfu_start_msg.info.img_info.crc16 = 0x2fDf;
  dfu_start_msg.info.img_info.major_ver = 1;
  dfu_start_msg.info.img_info.minor_ver = 7;
  dfu_start_msg.info.img_info.gitSHA = 0xdeadd00d;
  memcpy(evt.buf, &dfu_start_msg, sizeof(BcmpDfuStart));

  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(bcmp_tx_fake.arg1_history[0], BcmpDFUAckMessage);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUPayloadReqMessage);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientReceiving);

  evt.type = DfuEventChunkTimeout;
  evt.buf = NULL;
  evt.len = 0;
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // retry 1
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientReceiving);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // retry 2
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientReceiving);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // retry 3
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientReceiving);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // retry 4
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientReceiving);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // retry 5
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateError);
  evt.type = DfuEventNone;
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateIdle);
}

TEST_F(BcmpDfu, client_validate_fail) {
  // INIT SUCCESS
  bm_dfu_init();
  LibSmContext *ctx = bm_dfu_test_get_sm_ctx();
  BmDfuEvent evt = {
      .type = DfuEventInitSuccess,
      .buf = NULL,
      .len = 0,
  };
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateIdle);

  // DFU REQUEST
  evt.type = DfuEventReceivedUpdateRequest;
  evt.buf = (uint8_t *)malloc(sizeof(BcmpDfuStart));
  evt.len = sizeof(BcmpDfuStart);
  BcmpDfuStart dfu_start_msg;
  dfu_start_msg.header.frame_type = BcmpDFUStartMessage;
  dfu_start_msg.info.addresses.src_node_id = 0xbeefbeefdaadbaad;
  dfu_start_msg.info.addresses.dst_node_id = 0xdeadbeefbeeffeed;
  dfu_start_msg.info.img_info.image_size = IMAGE_SIZE;
  dfu_start_msg.info.img_info.chunk_size = CHUNK_SIZE;
  dfu_start_msg.info.img_info.crc16 = 0xDEAD; // bad CRC
  dfu_start_msg.info.img_info.major_ver = 1;
  dfu_start_msg.info.img_info.minor_ver = 7;
  dfu_start_msg.info.img_info.gitSHA = 0xdeadd00d;
  memcpy(evt.buf, &dfu_start_msg, sizeof(BcmpDfuStart));

  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(bcmp_tx_fake.arg1_history[0], BcmpDFUAckMessage);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUPayloadReqMessage);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientReceiving);

  // Chunk
  evt.type = DfuEventImageChunk;
  evt.buf = (uint8_t *)malloc(sizeof(BcmpDfuPayload) + CHUNK_SIZE);
  evt.len = sizeof(BcmpDfuPayload) + CHUNK_SIZE;
  BcmpDfuPayload dfu_payload_msg;
  dfu_payload_msg.header.frame_type = BcmpDFUPayloadMessage;
  dfu_payload_msg.chunk.addresses.src_node_id = 0xbeefbeefdaadbaad;
  dfu_payload_msg.chunk.addresses.dst_node_id = 0xdeadbeefbeeffeed;
  dfu_payload_msg.chunk.payload_length = CHUNK_SIZE;
  memcpy(evt.buf, &dfu_payload_msg, sizeof(dfu_payload_msg));
  memset(evt.buf + sizeof(dfu_payload_msg), 0xa5, CHUNK_SIZE);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // 512
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUPayloadReqMessage);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientReceiving);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // 1024
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUPayloadReqMessage);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientReceiving);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // 1536
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUPayloadReqMessage);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientReceiving);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // 2048
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientValidating);

  // Validating
  evt.type = DfuEventNone;
  evt.buf = NULL;
  evt.len = 0;
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx),
            BmDfuStateError); // FAILED to validate CRC
  evt.type = DfuEventNone;
  EXPECT_EQ(bm_dfu_get_error(), BmDfuErrBadCrc);
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateIdle);

  // DFU REQUEST
  evt.type = DfuEventReceivedUpdateRequest;
  evt.buf = (uint8_t *)malloc(sizeof(BcmpDfuStart));
  evt.len = sizeof(BcmpDfuStart);
  memcpy(evt.buf, &dfu_start_msg, sizeof(BcmpDfuStart));

  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(bcmp_tx_fake.arg1_history[0], BcmpDFUAckMessage);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUPayloadReqMessage);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientReceiving);

  // Chunk
  evt.type = DfuEventImageChunk;
  evt.buf = (uint8_t *)malloc(sizeof(BcmpDfuPayload) + CHUNK_SIZE);
  evt.len = sizeof(BcmpDfuPayload) + CHUNK_SIZE;
  memcpy(evt.buf, &dfu_payload_msg, sizeof(dfu_payload_msg));
  memset(evt.buf + sizeof(dfu_payload_msg), 0xa5, CHUNK_SIZE);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // 512
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUPayloadReqMessage);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientReceiving);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // 1024
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUPayloadReqMessage);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientReceiving);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // 1536
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUPayloadReqMessage);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientReceiving);
  dfu_payload_msg.chunk.payload_length = 1;
  memcpy(evt.buf, &dfu_payload_msg, sizeof(dfu_payload_msg));
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // 1537
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientValidating);

  // Validating
  evt.type = DfuEventNone;
  evt.buf = NULL;
  evt.len = 0;
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx),
            BmDfuStateError); // Image offest does not equal image length.
  evt.type = DfuEventNone;
  EXPECT_EQ(bm_dfu_get_error(), BmDfuErrMismatchLen);
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateIdle);
}

TEST_F(BcmpDfu, chunks_too_big) {
  // INIT SUCCESS
  bm_dfu_init();
  LibSmContext *ctx = bm_dfu_test_get_sm_ctx();
  BmDfuEvent evt = {
      .type = DfuEventInitSuccess,
      .buf = NULL,
      .len = 0,
  };
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateIdle);

  // DFU REQUEST
  evt.type = DfuEventReceivedUpdateRequest;
  evt.buf = (uint8_t *)malloc(sizeof(BcmpDfuStart));
  evt.len = sizeof(BcmpDfuStart);
  BcmpDfuStart dfu_start_msg;
  dfu_start_msg.header.frame_type = BcmpDFUStartMessage;
  dfu_start_msg.info.addresses.src_node_id = 0xbeefbeefdaadbaad;
  dfu_start_msg.info.addresses.dst_node_id = 0xdeadbeefbeeffeed;
  dfu_start_msg.info.img_info.image_size = IMAGE_SIZE;
  dfu_start_msg.info.img_info.chunk_size =
      CHUNK_SIZE * 100;                       // Chunk way too big
  dfu_start_msg.info.img_info.crc16 = 0xDEAD; // bad CRC
  dfu_start_msg.info.img_info.major_ver = 1;
  dfu_start_msg.info.img_info.minor_ver = 7;
  dfu_start_msg.info.img_info.gitSHA = 0xdeadd00d;
  memcpy(evt.buf, &dfu_start_msg, sizeof(BcmpDfuStart));

  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx),
            BmDfuStateError); // FAILED to validate CRC
  EXPECT_EQ(bm_dfu_get_error(), BmDfuErrChunkSize);
}

TEST_F(BcmpDfu, client_reboot_req_fail) {
  // INIT SUCCESS
  bm_dfu_init();
  LibSmContext *ctx = bm_dfu_test_get_sm_ctx();
  BmDfuEvent evt = {
      .type = DfuEventInitSuccess,
      .buf = NULL,
      .len = 0,
  };
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateIdle);

  // DFU REQUEST
  evt.type = DfuEventReceivedUpdateRequest;
  evt.buf = (uint8_t *)malloc(sizeof(BcmpDfuStart));
  evt.len = sizeof(BcmpDfuStart);
  BcmpDfuStart dfu_start_msg;
  dfu_start_msg.header.frame_type = BcmpDFUStartMessage;
  dfu_start_msg.info.addresses.src_node_id = 0xbeefbeefdaadbaad;
  dfu_start_msg.info.addresses.dst_node_id = 0xdeadbeefbeeffeed;
  dfu_start_msg.info.img_info.image_size = IMAGE_SIZE;
  dfu_start_msg.info.img_info.chunk_size = CHUNK_SIZE;
  dfu_start_msg.info.img_info.crc16 = 0x2fDf;
  dfu_start_msg.info.img_info.major_ver = 1;
  dfu_start_msg.info.img_info.minor_ver = 7;
  dfu_start_msg.info.img_info.gitSHA = 0xdeadd00d;
  memcpy(evt.buf, &dfu_start_msg, sizeof(BcmpDfuStart));

  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(bcmp_tx_fake.arg1_history[0], BcmpDFUAckMessage);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUPayloadReqMessage);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientReceiving);

  // Chunk
  evt.type = DfuEventImageChunk;
  evt.buf = (uint8_t *)malloc(sizeof(BcmpDfuPayload) + CHUNK_SIZE);
  evt.len = sizeof(BcmpDfuPayload) + CHUNK_SIZE;
  BcmpDfuPayload dfu_payload_msg;
  dfu_payload_msg.header.frame_type = BcmpDFUPayloadMessage;
  dfu_payload_msg.chunk.addresses.src_node_id = 0xbeefbeefdaadbaad;
  dfu_payload_msg.chunk.addresses.dst_node_id = 0xdeadbeefbeeffeed;
  dfu_payload_msg.chunk.payload_length = CHUNK_SIZE;
  memcpy(evt.buf, &dfu_payload_msg, sizeof(dfu_payload_msg));
  memset(evt.buf + sizeof(dfu_payload_msg), 0xa5, CHUNK_SIZE);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // 512
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUPayloadReqMessage);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientReceiving);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // 1024
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUPayloadReqMessage);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientReceiving);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // 1536
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUPayloadReqMessage);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientReceiving);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // 2048
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientValidating);

  // Validating
  evt.type = DfuEventNone;
  evt.buf = NULL;
  evt.len = 0;
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientRebootReq);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFURebootReqMessage);

  evt.type = DfuEventChunkTimeout;
  evt.buf = NULL;
  evt.len = 0;
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // retry 1
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientRebootReq);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // retry 2
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientRebootReq);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // retry 3
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientRebootReq);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // retry 4
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientRebootReq);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // retry 5
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateError);
  evt.type = DfuEventNone;
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateIdle);
}

TEST_F(BcmpDfu, reboot_done_fail) {
  // Set the reboot info.
  client_update_reboot_info.magic = DFU_REBOOT_MAGIC;
  client_update_reboot_info.host_node_id = 0xbeefbeefdaadbaad;
  client_update_reboot_info.major = 0; // Incorrect major
  client_update_reboot_info.minor = 7;
  client_update_reboot_info.gitSHA = 0xbaaddead;

  // INIT SUCCESS
  bm_dfu_init();
  LibSmContext *ctx = bm_dfu_test_get_sm_ctx();
  BmDfuEvent evt = {
      .type = DfuEventInitSuccess,
      .buf = NULL,
      .len = 0,
  };
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientRebootDone);
  EXPECT_EQ(bm_dfu_client_fail_update_and_reset_fake.call_count, 1);

  // Reset the bm_dfu_client_fail_update_and_reset_fake call count
  RESET_FAKE(bm_dfu_client_fail_update_and_reset);

  // Set the reboot info.
  client_update_reboot_info.magic = DFU_REBOOT_MAGIC;
  client_update_reboot_info.host_node_id = 0xbeefbeefdaadbaad;
  client_update_reboot_info.major = 1;
  client_update_reboot_info.minor = 7;
  client_update_reboot_info.gitSHA = 0xdeadd00d;
  git_sha_fake.return_val = 0xdeadd00d;

  // INIT SUCCESS
  bm_dfu_init();
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientRebootDone);
  EXPECT_EQ(bcmp_tx_fake.arg1_val, BcmpDFUBootCompleteMessage);
  evt.type = DfuEventChunkTimeout;
  evt.buf = NULL;
  evt.len = 0;
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // retry 1
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientRebootDone);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // retry 2
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientRebootDone);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // retry 3
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientRebootDone);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // retry 4
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientRebootDone);
  bm_dfu_test_set_dfu_event_and_run_sm(evt); // retry 5
  EXPECT_EQ(bm_dfu_client_fail_update_and_reset_fake.call_count, 1);
}

// Custom behavior to set the uint32_t pointer to 0
bool get_config_uint_custom(BmConfigPartition partition, const char *key,
                            size_t key_len, uint32_t *value) {
  if (value) {
    *value = 0;
  }
  return true;
}

TEST_F(BcmpDfu, client_confirm_skip) {
  // The config should be 0 to enable the "skip" feature.
  // So we need to have the get_config_uint function return 0.
  get_config_uint_fake.custom_fake = get_config_uint_custom;

  // Set the reboot info.
  client_update_reboot_info.magic = DFU_REBOOT_MAGIC;
  client_update_reboot_info.host_node_id = 0xbeefbeefdaadbaad;
  client_update_reboot_info.major = 1;
  client_update_reboot_info.minor = 7;
  client_update_reboot_info.gitSHA = 0xdeadd00d;
  git_sha_fake.return_val = 0xdeadd00d;

  // INIT SUCCESS
  bm_dfu_init();
  LibSmContext *ctx = bm_dfu_test_get_sm_ctx();
  BmDfuEvent evt = {
      .type = DfuEventInitSuccess,
      .buf = NULL,
      .len = 0,
  };
  bm_dfu_test_set_dfu_event_and_run_sm(evt);
  EXPECT_EQ(get_current_state_enum(ctx), BmDfuStateClientRebootDone);
  // The reboot info should now be cleared.
  EXPECT_EQ(client_update_reboot_info.magic, 0);
  EXPECT_EQ(client_update_reboot_info.host_node_id, 0);
  EXPECT_EQ(client_update_reboot_info.major, 0);
  EXPECT_EQ(client_update_reboot_info.minor, 0);
  EXPECT_EQ(client_update_reboot_info.gitSHA, 0);
  EXPECT_EQ(bm_dfu_client_set_confirmed_fake.call_count, 1);
  EXPECT_EQ(get_config_uint_fake.call_count, 1);
  EXPECT_EQ(set_config_uint_fake.call_count, 1);
  EXPECT_EQ(save_config_fake.call_count, 1);
}

// Testing private function, not in header but not marked static
extern "C" BmErr dfu_copy_and_process_message(BcmpProcessData data);

TEST_F(BcmpDfu, forward_ll_msg_for_other) {
  const uint64_t me = 0xdeadbeefbeeffeed;
  const uint64_t other = 0x7777beeffeed2222;

  // Forward link-local message not intended for this node
  BcmpDfuStart *fwd_start_msg = (BcmpDfuStart *)malloc(sizeof(BcmpDfuStart));
  fwd_start_msg->header.frame_type = BcmpDFUStartMessage;
  fwd_start_msg->info.addresses.dst_node_id = other;
  fwd_start_msg->info.addresses.src_node_id = 0xdeaddeaddeaddead;
  BcmpProcessData data = {
      .header = (BcmpHeader *)fwd_start_msg,
      .payload = (uint8_t *)fwd_start_msg,
      .size = sizeof(BcmpDfuStart),
      .dst = (void *)&multicast_ll_addr,
  };
  dfu_copy_and_process_message(data);
  EXPECT_EQ(bcmp_ll_forward_fake.call_count, 1);
  RESET_FAKE(bcmp_ll_forward);

  // Do not forward link-local message intended for this node
  fwd_start_msg->info.addresses.dst_node_id = me;
  dfu_copy_and_process_message(data);
  EXPECT_EQ(bcmp_ll_forward_fake.call_count, 0);

  // Do not forward global multicast message not intended for this node
  // Already forwarded in L2 before being passed to BCMP
  data.dst = (void *)&multicast_global_addr;
  fwd_start_msg->info.addresses.dst_node_id = other;
  dfu_copy_and_process_message(data);
  EXPECT_EQ(bcmp_ll_forward_fake.call_count, 0);

  free(fwd_start_msg);
}
