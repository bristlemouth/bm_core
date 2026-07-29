#include "gtest/gtest.h"
#include "fff.h"
DEFINE_FFF_GLOBALS;
extern "C" {
#include "bm_messages_helper.h"
#include "cbor.h"
#include "metrics_reply_msg.h"
}

// Build a "network_port_stats"-style component by hand (num_ports + one port's
// fields) and round-trip the whole envelope through encode + decode.
TEST(MetricsReplyMsg, EncodesAndDecodesEnvelope) {
  uint8_t num_ports = 1;
  uint8_t sqi_1 = 5;
  uint16_t mse_1 = 1234;

  BmEncoderTableEntry enc_fields[3];
  enc_fields[0] = {"num_ports", BM_FIELD_UINT8, &num_ports};
  enc_fields[1] = {"sqi_1", BM_FIELD_UINT8, &sqi_1};
  enc_fields[2] = {"mse_1", BM_FIELD_UINT16, &mse_1};

  MetricsComponent comp = {};
  comp.key = "network_port_stats";
  comp.fields = enc_fields;
  comp.num_fields = 3;

  MetricsReplyData d = {};
  d.version = METRICS_REPLY_VERSION;
  d.node_id = 0x0123456789abcdefULL;
  d.uptime_ms = 42000;
  d.components = &comp;
  d.num_components = 1;

  uint8_t buf[256];
  size_t len = 0;
  ASSERT_EQ(metrics_reply_encode(&d, buf, sizeof(buf), &len), CborNoError);
  ASSERT_GT(len, 0u);

  uint8_t got_num_ports = 0;
  uint8_t got_sqi = 0;
  uint16_t got_mse = 0;

  BmDecodeTableEntry dec_fields[3];
  dec_fields[0] = {"num_ports", BM_FIELD_UINT8, &got_num_ports};
  dec_fields[1] = {"sqi_1", BM_FIELD_UINT8, &got_sqi};
  dec_fields[2] = {"mse_1", BM_FIELD_UINT16, &got_mse};

  MetricsComponentDecode dcomp = {};
  dcomp.key = "network_port_stats";
  dcomp.fields = dec_fields;
  dcomp.num_fields = 3;

  MetricsReplyDecode out = {};
  out.components = &dcomp;
  out.num_components = 1;

  ASSERT_EQ(metrics_reply_decode(buf, len, &out), CborNoError);
  EXPECT_EQ(out.version, (uint8_t)METRICS_REPLY_VERSION);
  EXPECT_EQ(out.node_id, d.node_id);
  EXPECT_EQ(out.uptime_ms, d.uptime_ms);
  EXPECT_EQ(got_num_ports, num_ports);
  EXPECT_EQ(got_sqi, sqi_1);
  EXPECT_EQ(got_mse, mse_1);
}

// A requested component that isn't present in the reply is skipped, not an error.
TEST(MetricsReplyMsg, DecodeSkipsAbsentComponent) {
  MetricsReplyData d = {};
  d.version = METRICS_REPLY_VERSION;
  d.node_id = 1;
  d.uptime_ms = 0;
  d.components = NULL;
  d.num_components = 0;

  uint8_t buf[64];
  size_t len = 0;
  ASSERT_EQ(metrics_reply_encode(&d, buf, sizeof(buf), &len), CborNoError);

  uint8_t got_num_ports = 0xAA;
  BmDecodeTableEntry dec_fields[1];
  dec_fields[0] = {"num_ports", BM_FIELD_UINT8, &got_num_ports};

  MetricsComponentDecode dcomp = {};
  dcomp.key = "network_port_stats";
  dcomp.fields = dec_fields;
  dcomp.num_fields = 1;

  MetricsReplyDecode out = {};
  out.components = &dcomp;
  out.num_components = 1;

  ASSERT_EQ(metrics_reply_decode(buf, len, &out), CborNoError);
  EXPECT_EQ(got_num_ports, 0xAA); // untouched: component absent
}
