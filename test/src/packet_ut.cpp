#include <gtest/gtest.h>

class packet_test : public ::testing::Test {
  packet_test() {}

  ~packet_test() override {}

  void Setup() override { bcmp_parser_init(); }

  void TearDown() override {}
};

// Demonstrate some basic assertions.
TEST_F(packet_test, parse_packets) {
  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "world");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
}
