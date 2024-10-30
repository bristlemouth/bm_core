#include "fff.h"
#include "gtest/gtest.h"

DEFINE_FFF_GLOBALS;

extern "C" {
#include "aligned_malloc.h"
}

TEST(AlignedMalloc, happy_path) {
  const char buffer[] =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
      "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";
  const size_t buffer_size = sizeof(buffer);
  void *ptr = aligned_malloc(4, buffer_size);
  EXPECT_NE(ptr, nullptr);
  EXPECT_EQ((uintptr_t)ptr % 4, 0);
  memcpy(ptr, buffer, buffer_size);
  EXPECT_STREQ((const char *)ptr, buffer);
  aligned_free(ptr);
}

TEST(AlignedMalloc, alignment_must_be_power_of_two) {
  void *ptr = aligned_malloc(3, 16);
  EXPECT_EQ(ptr, nullptr);
  aligned_free(ptr);
  ptr = aligned_malloc(5, 16);
  EXPECT_EQ(ptr, nullptr);
  aligned_free(ptr);
  ptr = aligned_malloc(6, 16);
  EXPECT_EQ(ptr, nullptr);
  aligned_free(ptr);
  ptr = aligned_malloc(7, 16);
  EXPECT_EQ(ptr, nullptr);
  aligned_free(ptr);
}

TEST(AlignedMalloc, size_zero_not_allowed) {
  void *ptr = aligned_malloc(16, 0);
  EXPECT_EQ(ptr, nullptr);
  aligned_free(ptr);
}

TEST(AlignedMalloc, alignment_zero_not_allowed) {
  void *ptr = aligned_malloc(0, 16);
  EXPECT_EQ(ptr, nullptr);
  aligned_free(ptr);
}

TEST(AlignedMalloc, align8) {
  void *ptr = aligned_malloc(8, 37);
  EXPECT_NE(ptr, nullptr);
  EXPECT_EQ((uintptr_t)ptr % 8, 0);
  aligned_free(ptr);
}

TEST(AlignedMalloc, align16) {
  void *ptr = aligned_malloc(16, 37);
  EXPECT_NE(ptr, nullptr);
  EXPECT_EQ((uintptr_t)ptr % 16, 0);
  aligned_free(ptr);
}

TEST(AlignedMalloc, align32) {
  void *ptr = aligned_malloc(32, 37);
  EXPECT_NE(ptr, nullptr);
  EXPECT_EQ((uintptr_t)ptr % 32, 0);
  aligned_free(ptr);
}

TEST(AlignedMalloc, align64) {
  void *ptr = aligned_malloc(64, 37);
  EXPECT_NE(ptr, nullptr);
  EXPECT_EQ((uintptr_t)ptr % 64, 0);
  aligned_free(ptr);
}

TEST(AlignedMalloc, align128) {
  void *ptr = aligned_malloc(128, 37);
  EXPECT_NE(ptr, nullptr);
  EXPECT_EQ((uintptr_t)ptr % 128, 0);
  aligned_free(ptr);
}

TEST(AlignedMalloc, align256) {
  void *ptr = aligned_malloc(256, 37);
  EXPECT_NE(ptr, nullptr);
  EXPECT_EQ((uintptr_t)ptr % 256, 0);
  aligned_free(ptr);
}
