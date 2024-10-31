#include "gtest/gtest.h"

extern "C" {
#include "util.h"
}

// The fixture for testing class Foo.
class TimeRemaining : public ::testing::Test {
 protected:
  // You can remove any or all of the following functions if its body
  // is empty.

  TimeRemaining() {
     // You can do set-up work for each test here.
  }

  ~TimeRemaining() override {
     // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  void SetUp() override {
     // Code here will be called immediately after the constructor (right
     // before each test).
  }

  void TearDown() override {
     // Code here will be called immediately after each test (right
     // before the destructor).
  }

  // Objects declared here can be used by all tests in the test suite for Foo.
};


TEST_F(TimeRemaining, all_zeroes)
{
  uint32_t rem = time_remaining(0, 0, 0);
  EXPECT_EQ(rem, 0);
}

TEST_F(TimeRemaining, basic_rem)
{
  uint32_t rem = time_remaining(0, 5, 10);
  EXPECT_EQ(rem, 5);
}

TEST_F(TimeRemaining, basic_no_rem_exact)
{
  uint32_t rem = time_remaining(0, 10, 10);
  EXPECT_EQ(rem, 0);
}

TEST_F(TimeRemaining, basic_no_rem_over)
{
  uint32_t rem = time_remaining(0, 15, 10);
  EXPECT_EQ(rem, 0);
}

//
// Test uint32_t overflow
//
TEST_F(TimeRemaining, big)
{
  uint32_t rem = time_remaining(UINT32_MAX - 10, UINT32_MAX - 5, 10);
  EXPECT_EQ(rem, 5);
}

TEST_F(TimeRemaining, big_no_rem)
{
  uint32_t rem = time_remaining(UINT32_MAX - 10, UINT32_MAX, 10);
  EXPECT_EQ(rem, 0);
}

TEST_F(TimeRemaining, big_some_rem_over)
{
  uint32_t rem = time_remaining(UINT32_MAX - 10, UINT32_MAX, 15);
  EXPECT_EQ(rem, 5);
}

TEST_F(TimeRemaining, big_some_rem_over2)
{
  uint32_t rem = time_remaining(UINT32_MAX - 10, 3, 15);
  EXPECT_EQ(rem, 1);
}

TEST_F(TimeRemaining, big_no_rem_over)
{
  uint32_t rem = time_remaining(UINT32_MAX - 10, 3, 10);
  EXPECT_EQ(rem, 0);
}

TEST_F(TimeRemaining, big_no_rem_over2)
{
  uint32_t rem = time_remaining(UINT32_MAX - 10, 200, 15);
  EXPECT_EQ(rem, 0);
}
