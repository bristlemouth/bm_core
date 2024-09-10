#include <gtest/gtest.h>
#include <helpers.hpp>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "packet.h"
#ifdef __cplusplus
}
#endif

static rnd_gen RND;

class packet_test : public ::testing::Test {
protected:
  packet_test() {}

  ~packet_test() override {}

  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(packet_test, serialize) {
  BmErr err = BmOK;
  uint8_t *buf = NULL;

  // Test General Header
  //BCMPHeader header = {
  //    (uint16_t)RND.rnd_int(UINT16_MAX, 0), (uint16_t)RND.rnd_int(UINT16_MAX, 0),
  //    (uint8_t)RND.rnd_int(UINT8_MAX, 0),   (uint8_t)RND.rnd_int(UINT8_MAX, 0),
  //    (uint32_t)RND.rnd_int(UINT32_MAX, 0), (uint8_t)RND.rnd_int(UINT8_MAX, 0),
  //    (uint8_t)RND.rnd_int(UINT8_MAX, 0),   (uint8_t)RND.rnd_int(UINT8_MAX, 0),
  //};
  //buf = (uint8_t *)&header;

  //err = serialize(buf, BCMP_HEADER);
  //ASSERT_EQ(err, BmOK);
  //ASSERT_EQ(memcmp(buf, &header, sizeof(header)), 0);
}
