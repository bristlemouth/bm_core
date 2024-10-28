#include <gtest/gtest.h>
#include <helpers.hpp>

#include "fff.h"

DEFINE_FFF_GLOBALS;

extern "C" {
#include "echo_service.h"
}

class EchoService : public ::testing::Test {
public:
private:
protected:
  EchoService() {}
  ~EchoService() override {}

  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(EchoService, test_init) { echo_service_init(); }
