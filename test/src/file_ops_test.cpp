#include <gtest/gtest.h>
#include <helpers.hpp>

#include "fff.h"

DEFINE_FFF_GLOBALS;

#include "bcmp.h"
#include "file_ops.h"
#include "mock_pubsub.h"

class FileOps : public ::testing::Test {
public:
  rnd_gen RND;

private:
protected:
  FileOps() {}
  ~FileOps() override {}
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(FileOps, fappend) {
  uint64_t node_id = RND.rnd_int(UINT64_MAX, UINT8_MAX);
  const char *file_name = "hello_world.txt";
  uint8_t buf[UINT8_MAX];
  RND.rnd_array(buf, UINT8_MAX);

  // Test proper use cases
  bm_pub_fake.return_val = BmOK;
  ASSERT_EQ(bm_file_append(node_id, file_name, buf, array_size(buf)), BmOK);

  // Test improper use cases
  bm_pub_fake.return_val = BmEBADMSG;
  ASSERT_EQ(bm_file_append(node_id, file_name, buf, array_size(buf)),
            BmEBADMSG);
  ASSERT_EQ(bm_file_append(node_id, NULL, buf, array_size(buf)), BmEINVAL);
  ASSERT_EQ(bm_file_append(node_id, file_name, NULL, 0), BmEINVAL);
}
