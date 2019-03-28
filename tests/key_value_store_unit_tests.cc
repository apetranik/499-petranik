#include <optional>

#include <gtest/gtest.h>

#include "../key_value_store.h"

// Put value in KVS and then retrieve
TEST(PutGetTest, Basic) {
  KeyValueStore test_store;
  bool success = test_store.Put("testkey", "testvalue");

  auto optional_reply = test_store.Get("testkey");
  ASSERT_EQ(true, success);
  ASSERT_EQ("testvalue", optional_reply);
}
// Attempts to Get non-existent key
TEST(GetTest, EmptyKey) {
  KeyValueStore test_store;
  auto optional_reply = test_store.Get("");
  ASSERT_EQ("[empty_key]", optional_reply);
}

// Attempts to Put an empty key
TEST(PutTest, EmptyKey) {
  KeyValueStore test_store;
  bool success = test_store.Put("", "testvalue");
  ASSERT_EQ(false, success);
}
// Test updating a value for a key, value pair that already exists in KVS
TEST(PutTest, UpdateValue) {
  KeyValueStore test_store;
  test_store.Put("testkey", "testvalue1");
  test_store.Put("testkey", "testvalue2");

  auto optional_reply = test_store.Get("testkey");
  ASSERT_EQ("testvalue2", optional_reply);
}

// Puts value in KVS, then deletes,
TEST(DeleteTest, PutThenDetele) {
  KeyValueStore test_store;
  test_store.Put("testkey", "testvalue");

  auto optional_reply = test_store.Get("testkey");
  ASSERT_EQ("testvalue", optional_reply);

  // if optional is null, optional_reply will = "[empty_key]"
  bool success = test_store.Del("testkey");
  optional_reply = test_store.Get("testkey");
  ASSERT_EQ(true, success);
  ASSERT_EQ("[empty_key]", optional_reply);
}
// Puts value in KVS, then deletes,
TEST(DeleteTest, NonExistentKey) {
  KeyValueStore test_store;
  bool success = test_store.Del("non-existent");
  ASSERT_EQ(false, success);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
