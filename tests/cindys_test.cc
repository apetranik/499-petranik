#include <algorithm>
#include <string>

#include <gtest/gtest.h>

#include "../src/key_value_store.h"
#include "../src/key_value_store_interface.h"
#include "../src/service.h"
// Test CheckIfHaveHashtag()- checks if a chirp has a hashtag
TEST(TestCheckIfHaveHashtagfunction, Basic) {
  KeyValueStore kvs;
  ServiceLayer sl(&kvs);
  bool register_success = sl.registeruser("testuser");
  ASSERT_EQ(true, register_success);
  sl.chirp("testuser", "#hi", "");
  std::string hashtag = sl.CheckIfHaveHashtag("#hi");
  ASSERT_EQ("#hi", hashtag);
}
// Test more complex chirping with hashtag
TEST(ComplexChirpWithHashTag, Simple) {
  KeyValueStore kvs;
  ServiceLayer sl(&kvs);
  bool register_success = sl.registeruser("testuser");
  ASSERT_EQ(true, register_success);
  sl.chirp("testuser", "omg #hi there", "");
  std::string hashtag = sl.CheckIfHaveHashtag("omg #hi there");
  ASSERT_EQ("#hi", hashtag);
}
// Test chirp doesn't have hashtag
TEST(ChirpNoHashtag, Simple) {
  KeyValueStore kvs;
  ServiceLayer sl(&kvs);
  bool register_success = sl.registeruser("testuser");
  ASSERT_EQ(true, register_success);
  sl.chirp("testuser", "no hashtag", "");
  std::string hashtag = sl.CheckIfHaveHashtag("no hashtag");
  ASSERT_EQ("", hashtag);
}
// Test chirp with multiple hashtags
TEST(MultipleHashtags, Simple) {
  KeyValueStore kvs;
  ServiceLayer sl(&kvs);
  bool register_success = sl.registeruser("testuser");
  ASSERT_EQ(true, register_success);
  sl.chirp("testuser", "#hi #there", "");
  auto hashtag = sl.CheckIfHaveMultipleHashtags("#hi #there");
  ASSERT_EQ(2, hashtag.size());
}
int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
