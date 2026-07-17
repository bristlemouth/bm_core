#include "gtest/gtest.h"
#include "fff.h"
DEFINE_FFF_GLOBALS;
extern "C" {
    #include "cbor.h"
    #include "metrics_reply_msg.h"
}

TEST(MetricsReplyMsg, EncodesAndParsesBack) {
    MetricsReplyData d = {};
    d.version = METRICS_REPLY_VERSION;
    d.node_id = 0x0123456789abcdefULL;
    d.uptime_ms = 42000;
    d.num_ports = 2;
    d.ports[0] = {12, 5, 2, 3, 7};   // mse, sqi, lq, rxe, sye
    d.ports[1] = {99, 1, 0, 0, 0};

    uint8_t buf[256];
    size_t len = 0;
    ASSERT_EQ(metrics_reply_encode(&d, buf, sizeof(buf), &len), CborNoError);
    ASSERT_GT(len, 0u);

    CborParser parser; CborValue map, v;
    ASSERT_EQ(cbor_parser_init(buf, len, 0, &parser, &map), CborNoError);
    ASSERT_TRUE(cbor_value_is_map(&map));
    size_t nfields = 0;
    cbor_value_get_map_length(&map, &nfields);
    EXPECT_EQ(nfields, (size_t)METRICS_REPLY_NUM_FIELDS);

    uint64_t got = 0;
    ASSERT_EQ(cbor_value_map_find_value(&map, "node", &v), CborNoError);
    ASSERT_EQ(cbor_value_get_uint64(&v, &got), CborNoError);
    EXPECT_EQ(got, d.node_id);
    ASSERT_EQ(cbor_value_map_find_value(&map, "v", &v), CborNoError);
    cbor_value_get_uint64(&v, &got);
    EXPECT_EQ(got, (uint64_t)METRICS_REPLY_VERSION);
}

TEST(MetricsReplyMsg, ZeroPortsIsValid) {
    MetricsReplyData d = {};
    d.version = METRICS_REPLY_VERSION;
    d.num_ports = 0;
    uint8_t buf[64]; size_t len = 0;
    EXPECT_EQ(metrics_reply_encode(&d, buf, sizeof(buf), &len), CborNoError);
}