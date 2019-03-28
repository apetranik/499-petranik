#ifndef ADD_H
#define ADD_H
#include <google/protobuf/util/time_util.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <string>

#include "../service.h"
#include "../service_controller.h"

// Register user and confirm their existence
TEST(RegisterUserTest, Basic) {
  KeyValueStore kvs;
  ServiceLayer sl(&kvs);
  bool register_success = sl.registeruser("testuser");
  ASSERT_EQ(true, register_success);
  std::string user = kvs.Get("testuser").value();
  ASSERT_EQ(true, !user.empty());
}
// Register empty user
TEST(RegisterUserTest, EmptyUser) {
  KeyValueStore kvs;
  ServiceLayer sl(&kvs);
  bool register_success = sl.registeruser("");
  ASSERT_EQ(false, register_success);
}
// Registers a user twice
TEST(RegisterUserTest, DuplicateRegister) {
  KeyValueStore kvs;
  ServiceLayer sl(&kvs);
  bool register_success = sl.registeruser("testuser");
  ASSERT_EQ(true, register_success);
  std::string user = kvs.Get("testuser").value();
  ASSERT_EQ(true, !user.empty());
  // duplicate register
  register_success = sl.registeruser("testuser");
  ASSERT_EQ(false, register_success);
}
// Register user and make basic chirp
TEST(ChirpTest, RegisterAndBasicChirp) {
  KeyValueStore kvs;
  ServiceLayer sl(&kvs);
  bool register_success = sl.registeruser("testuser");
  ASSERT_EQ(true, register_success);
  std::string user = kvs.Get("testuser").value();
  ASSERT_EQ(true, !user.empty());

  auto chirp_str = sl.chirp("testuser", "testchirp", "");
  chirp::Chirp chirp = chirp_str.value();
  ASSERT_EQ("testuser", chirp.username());
  ASSERT_EQ("testchirp", chirp.text());
  ASSERT_EQ("", chirp.parent_id());
}
// Register user and reply to a chirp from self
TEST(ChirpTest, RegisterAndSimpleReplyChirp) {
  KeyValueStore kvs;
  ServiceLayer sl(&kvs);
  bool register_success = sl.registeruser("testuser");
  ASSERT_EQ(true, register_success);
  std::string user = kvs.Get("testuser").value();
  ASSERT_EQ(true, !user.empty());
  // Chirp 1
  auto chirp_str = sl.chirp("testuser", "testchirp1", "");
  chirp::Chirp chirp = chirp_str.value();
  ASSERT_EQ("testuser", chirp.username());
  ASSERT_EQ("testchirp1", chirp.text());
  ASSERT_EQ("", chirp.parent_id());
  // Chirp 2
  auto chirp_str2 = sl.chirp("testuser", "testchirp2", chirp.id());
  chirp::Chirp chirp2 = chirp_str2.value();
  ASSERT_EQ("testuser", chirp2.username());
  ASSERT_EQ("testchirp2", chirp2.text());
  ASSERT_EQ(chirp.id(), chirp2.parent_id());
}
// Chirp from nonexistent user
TEST(ChirpTest, ChirpFromNullUser) {
  KeyValueStore kvs;
  ServiceLayer sl(&kvs);
  // Chirp 1
  auto chirp_str = sl.chirp("testuser", "testchirp", "");
  bool isEmpty = !chirp_str;
  ASSERT_TRUE(isEmpty);
}
// Reply to to nonexistant parent id
TEST(ChirpTest, ReplyToNullParent) {
  KeyValueStore kvs;
  ServiceLayer sl(&kvs);
  bool register_success = sl.registeruser("testuser");
  ASSERT_EQ(true, register_success);
  std::string user = kvs.Get("testuser").value();
  ASSERT_EQ(true, !user.empty());
  // Chirp 1
  auto chirp_str = sl.chirp("testuser", "testchirp", "1000");
  bool isEmpty = !chirp_str;
  ASSERT_TRUE(isEmpty);
}
// Register 2 users who reply to each other
TEST(ChirpTest, RegisterAndReplyChirp) {
  KeyValueStore kvs;
  ServiceLayer sl(&kvs);
  // User 1
  bool register_success = sl.registeruser("testuser1");
  ASSERT_EQ(true, register_success);
  std::string user = kvs.Get("testuser1").value();
  ASSERT_EQ(true, !user.empty());
  // Chirp 1
  auto chirp_str = sl.chirp("testuser1", "testchirp1", "");
  chirp::Chirp chirp = chirp_str.value();
  ASSERT_EQ("testuser1", chirp.username());
  ASSERT_EQ("testchirp1", chirp.text());
  ASSERT_EQ("", chirp.parent_id());
  // User 2
  register_success = sl.registeruser("testuser2");
  ASSERT_EQ(true, register_success);
  user = kvs.Get("testuser2").value();
  ASSERT_EQ(true, !user.empty());
  // Chirp 2
  auto chirp_str2 = sl.chirp("testuser2", "testchirp2", chirp.id());
  chirp::Chirp chirp2 = chirp_str2.value();
  ASSERT_EQ("testuser2", chirp2.username());
  ASSERT_EQ("testchirp2", chirp2.text());
  ASSERT_EQ(chirp.id(), chirp2.parent_id());
}
// Register 2 users and one follows the other
TEST(FollowTest, RegisterAndBasicFollow) {
  KeyValueStore kvs;
  ServiceLayer sl(&kvs);
  // User 1
  bool register_success = sl.registeruser("testuser1");
  ASSERT_EQ(true, register_success);
  std::string user = kvs.Get("testuser1").value();
  ASSERT_EQ(true, !user.empty());
  // User 2
  register_success = sl.registeruser("testuser2");
  ASSERT_EQ(true, register_success);
  user = kvs.Get("testuser2").value();
  ASSERT_EQ(true, !user.empty());

  std::string msg = sl.follow("testuser1", "testuser2");
  ASSERT_EQ("success", msg);
}
// Registers 1 user who attempts to follow nonexistent 2nd user
TEST(FollowTest, RegisterAndNullFollowAttempt) {
  KeyValueStore kvs;
  ServiceLayer sl(&kvs);
  // User 1
  bool register_success = sl.registeruser("testuser1");
  ASSERT_EQ(true, register_success);
  std::string user = kvs.Get("testuser1").value();
  ASSERT_EQ(true, !user.empty());
  std::string msg = sl.follow("testuser1", "nonexist");
  ASSERT_EQ("followee_error", msg);
}
// Non existent user attempts to follow user that DOES** exist
TEST(FollowTest, FollowNonExistUser) {
  KeyValueStore kvs;
  ServiceLayer sl(&kvs);
  // User 1
  bool register_success = sl.registeruser("testuser");
  ASSERT_EQ(true, register_success);
  std::string user = kvs.Get("testuser").value();
  ASSERT_EQ(true, !user.empty());
  std::string msg = sl.follow("nonexist", "testuser");
  ASSERT_EQ("user_error", msg);
}
// User tries to follow self
TEST(FollowTest, SelfFollow) {
  KeyValueStore kvs;
  ServiceLayer sl(&kvs);
  // User 1
  bool register_success = sl.registeruser("testuser");
  ASSERT_EQ(true, register_success);
  std::string user = kvs.Get("testuser").value();
  ASSERT_EQ(true, !user.empty());
  std::string msg = sl.follow("testuser", "testuser");
  ASSERT_EQ("self_follow", msg);
}
// User tries to follow same user 2x
TEST(FollowTest, DuplicateFollow) {
  KeyValueStore kvs;
  ServiceLayer sl(&kvs);
  // User 1
  bool register_success = sl.registeruser("testuser1");
  ASSERT_EQ(true, register_success);
  std::string user = kvs.Get("testuser1").value();
  ASSERT_EQ(true, !user.empty());
  // User 2
  register_success = sl.registeruser("testuser2");
  ASSERT_EQ(true, register_success);
  user = kvs.Get("testuser2").value();
  ASSERT_EQ(true, !user.empty());
  // 1st follow attempt
  std::string msg = sl.follow("testuser1", "testuser2");
  ASSERT_EQ("success", msg);
  // 2nd follow attempt
  msg = sl.follow("testuser1", "testuser2");
  ASSERT_EQ("dup_follow", msg);
}
// Register user and make basic chirp
TEST(ReadTest, BasicChirpAndRead) {
  KeyValueStore kvs;
  ServiceLayer sl(&kvs);
  // Register
  bool register_success = sl.registeruser("testuser");
  ASSERT_EQ(true, register_success);
  std::string user = kvs.Get("testuser").value();
  ASSERT_EQ(true, !user.empty());
  // Chirp
  auto chirp_str = sl.chirp("testuser", "testchirp", "");
  chirp::Chirp chirp = chirp_str.value();
  ASSERT_EQ("testuser", chirp.username());
  ASSERT_EQ("testchirp", chirp.text());
  ASSERT_EQ("", chirp.parent_id());
  // Read
  std::vector<chirp::Chirp> read_chirp = sl.read(chirp.id());
  ASSERT_EQ(1, read_chirp.size());
  ASSERT_EQ("testchirp", read_chirp[0].text());
}
// Register user and make basic chirp
TEST(ReadTest, ReplyChirpAndRead) {
  KeyValueStore kvs;
  ServiceLayer sl(&kvs);
  // Register
  bool register_success = sl.registeruser("testuser1");
  ASSERT_EQ(true, register_success);
  std::string user = kvs.Get("testuser1").value();
  ASSERT_EQ(true, !user.empty());
  // Chirp
  auto chirp_str = sl.chirp("testuser1", "testchirp1", "");
  chirp::Chirp chirp = chirp_str.value();
  ASSERT_EQ("testuser1", chirp.username());
  ASSERT_EQ("testchirp1", chirp.text());
  ASSERT_EQ("", chirp.parent_id());
  // User 2
  register_success = sl.registeruser("testuser2");
  ASSERT_EQ(true, register_success);
  user = kvs.Get("testuser2").value();
  ASSERT_EQ(true, !user.empty());
  // Chirp 2
  auto chirp_str2 = sl.chirp("testuser2", "testchirp2", chirp.id());
  chirp::Chirp chirp2 = chirp_str2.value();
  ASSERT_EQ("testuser2", chirp2.username());
  ASSERT_EQ("testchirp2", chirp2.text());
  ASSERT_EQ(chirp.id(), chirp2.parent_id());
  // Read
  std::vector<chirp::Chirp> read_chirps = sl.read(chirp.id());
  ASSERT_EQ(2, read_chirps.size());
  ASSERT_EQ("testchirp1", read_chirps[0].text());
  ASSERT_EQ(chirp.id(), read_chirps[0].id());
  ASSERT_EQ("testchirp2", read_chirps[1].text());
  ASSERT_EQ(chirp2.id(), read_chirps[1].id());
}
/*


// register two users, have the first follow the second, get a current
// timestamp, second user chirps twice, monitor starting from timestamp
TEST(FollowingUserTest, FollowMonitor) {
  KeyValueStore kvs;
  ServiceLayer sl(&kvs);
  bool register_success = sl.registeruser("testUser");
  ASSERT_EQ(true, register_success);
  register_success = sl.registeruser("testUserToFollow");
  ASSERT_EQ(true, register_success);

  const std::deque<std::string>& user_value = kvs.get("0testUser");
  ASSERT_EQ(1, user_value.size());
  const std::deque<std::string>& user_value2 =
      kvs.get("0testUserToFollow");
  ASSERT_EQ(1, user_value2.size());

  bool follow_success = sl.follow("testUser",
"testUserToFollow"); ASSERT_EQ(true, follow_success); const
std::deque<std::string>& user_following = kvs.get("1testUser");
  ASSERT_EQ(2, user_following.size());

  chirp::Timestamp initial_time;
  int64_t seconds = google::protobuf::util::TimeUtil::TimestampToSeconds(
      google::protobuf::util::TimeUtil::GetEpoch());
  int64_t useconds = google::protobuf::util::TimeUtil::TimestampToMicroseconds(
      google::protobuf::util::TimeUtil::GetEpoch());
  initial_time.set_seconds(seconds);
  initial_time.set_useconds(useconds);

  sl.chirp("testUserToFollow", "testText", "");
  sl.chirp("testUserToFollow", "testText2", "");
  std::deque<chirp::Chirp> monitored_chirps =
      sl.monitor("testUser", initial_time);
  ASSERT_EQ(2, monitored_chirps.size());
  ASSERT_EQ("testText", monitored_chirps.at(0).text());
  ASSERT_EQ("testText2", monitored_chirps.at(1).text());
}

// try to follow a user that does not exist
TEST(FollowingUserTest, FollowNonexistingUser) {
  KeyValueStore kvs;
  ServiceLayer sl(&kvs);
  bool register_success = sl.registeruser("testUser");
  ASSERT_EQ(true, register_success);
  bool follow_success = sl.follow("testUser",
"testUserToFollow"); ASSERT_EQ(false, follow_success);
}

// chirp with invalid parent id
TEST(ChirpTest, InvalidParent) {
  KeyValueStore kvs;
  ServiceLayer sl(&kvs);
  bool register_success = sl.registeruser("testUser");
  ASSERT_EQ(true, register_success);
  chirp::Chirp generated_chirp =
      sl.chirp("testUser", "hello world", "FAKE_ID");
  ASSERT_EQ("ERROR", generated_chirp.id());
} */

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
#endif
