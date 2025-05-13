#include "gtest/gtest.h"

extern "C" {
#include "util.h"
}

class Util : public ::testing::Test {

protected:
  Util() {}
  ~Util() override {}
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(Util, wildcard_match) {
  EXPECT_TRUE(bm_wildcard_match("aaaa", 4, "a*a", 3));

  EXPECT_TRUE(bm_wildcard_match("aaabxc_file.txt", 15, "*a*b?c*.txt", 11));
  EXPECT_TRUE(bm_wildcard_match("xaxbxc_something.txt", 20, "*a*b?c*.txt", 11));

  EXPECT_FALSE(bm_wildcard_match("alpha_betaXc123.txt", 19, "*a*b?c*.txt", 11));
  EXPECT_TRUE(bm_wildcard_match("alpha_betaXc123.txt", 19, "*a*b*c*.txt", 11));

  EXPECT_TRUE(
      bm_wildcard_match("report-2023-Xsummary", 20, "report-????-*y", 14));
  EXPECT_TRUE(bm_wildcard_match("report-1925-diary", 17, "report-????-*y", 14));
  EXPECT_FALSE(bm_wildcard_match("report-2023-Xbad", 16, "report-????-*y", 14));
}
