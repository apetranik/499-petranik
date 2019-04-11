#include "service.h"

ServiceLayer::ServiceLayer(KeyValueStoreInterface *kvs_connection) {
  backend_client_ = kvs_connection;
}
// Constructs a RegisterRequest and sends to service layer thru grpc and
// receives a RegisterReply back
bool ServiceLayer::registeruser(const std::string &username) {

  std::string user_serialized = backend_client_->Get(username).value();

  // user already exists
  if (user_serialized != "[empty_key]") {
    return false;
  }
  // User doesn't exist, so save to backend
  chirp::User user;
  user.set_username(username);
  user.SerializeToString(&user_serialized);
  bool success = backend_client_->Put(username, user_serialized);
  return success;
}
std::string ServiceLayer::CheckIfHaveHashtag(const std::string &text) {

  std::size_t found = text.find('#');
  std::string hashtagword = "";
  if (found != std::string::npos) {
    int position = static_cast<int>(found);
    for (int i = position; i < text.length(); i++) {
      if (text[i] != ' ') {
        hashtagword += text[i];
      } else {
        break;
      }
    }
    hashtagword = trim(hashtagword);
    return hashtagword;
  }
}
std::string ServiceLayer::trim(std::string &str) {
  size_t first = str.find_first_not_of(' ');
  if (first == std::string::npos)
    return "";
  size_t last = str.find_last_not_of(' ');
  return str.substr(first, (last - first + 1));
}
// Constructs a ChirpRequest and sends to service layer thru grpc and receives
// a ChirpReply back
std::optional<chirp::Chirp> ServiceLayer::chirp(const std::string &username,
                                                const std::string &text,
                                                const std::string &parent_id) {
  std::lock_guard<std::mutex> guard(mutex_); // get lock

  std::string hashtag = CheckIfHaveHashtag(text);
  // check if user who is chirping exists in key value store
  auto user_exists = backend_client_->Get(username);
  if (user_exists == "[empty_key]") {
    return {};
  }
  // check for parent id's existance
  if (!parent_id.empty()) {
    std::string parent_exists = backend_client_->Get(parent_id).value();
    if (parent_exists == "[empty_key]") {
      return {};
    }
  }
  // User does exist, so allow them to chirp
  // Create new chirp ID based on current number of chirps
  std::string num_chirps_str = backend_client_->Get("num_chirps").value();
  int num_chirps;
  if (num_chirps_str == "[empty_key]") {
    num_chirps = 0;
  } else {
    num_chirps = std::stoi(num_chirps_str);
  }
  ++num_chirps;
  num_chirps_str = std::to_string(num_chirps);
  bool success = backend_client_->Put("num_chirps",
                                      num_chirps_str); // update new chirp count

  // construct new Chirp and fill with relevant params
  chirp::Chirp chirp;
  chirp.set_username(username);
  chirp.set_text(text);
  chirp.set_id(num_chirps_str);

  if (!parent_id.empty()) {
    chirp.set_parent_id(parent_id);
  }

  // generate time stamp
  chirp::Timestamp *stamp = new chirp::Timestamp();
  int64_t seconds = google::protobuf::util::TimeUtil::TimestampToSeconds(
      google::protobuf::util::TimeUtil::GetCurrentTime());
  int64_t useconds = google::protobuf::util::TimeUtil::TimestampToMicroseconds(
      google::protobuf::util::TimeUtil::GetCurrentTime());
  stamp->set_seconds(seconds);
  stamp->set_useconds(useconds);
  chirp.set_allocated_timestamp(stamp);

  std::string chirp_serialized;
  chirp.SerializeToString(&chirp_serialized); // Serialize chirp to string
  backend_client_->Put(chirp.id(),
                       chirp_serialized); // add new chirp to backend

  // If applicable, update Parent Chirp with new child reply
  if (!parent_id.empty()) {
    std::string parent_chirp_serialized =
        backend_client_->Get(parent_id).value();
    chirp::Chirp parent_chirp;
    parent_chirp.ParseFromString(parent_chirp_serialized);
    std::string *new_child_reply_id = parent_chirp.add_child_ids();
    *new_child_reply_id = chirp.id();
    parent_chirp.SerializeToString(&parent_chirp_serialized);
    backend_client_->Put(parent_id, parent_chirp_serialized);
  }

  // Check for monitoring users, if any, send new chirp to monitoring followers
  chirp::User user;
  std::string user_serialized = backend_client_->Get(username).value();
  user.ParseFromString(user_serialized);

  std::vector<std::string> monitoring_users_serialized; // followers_monitoring
  std::vector<chirp::User> monitoring_users;
  std::copy(user.followers_monitoring().begin(),
            user.followers_monitoring().end(),
            std::back_inserter(monitoring_users_serialized));

  std::string monitoring_user_serialized;
  chirp::User monitoring_user;

  // iterate thru all currently monitoring users and give them new chirp
  for (const std::string username : monitoring_users_serialized) {
    monitoring_user_serialized =
        backend_client_->Get(username).value(); // Get follower
    monitoring_user.ParseFromString(monitoring_user_serialized);
    chirp::Chirp *incoming_chirp =
        monitoring_user.add_monitored_chirps(); // add new chirp to be monitored
    *incoming_chirp = chirp;                    // fill chirp
    monitoring_user.SerializeToString(&monitoring_user_serialized);
    backend_client_->Put(username,
                         monitoring_user_serialized); // save chirp to follower
  }

  // add to hashtag if exist, not empty
  if (hashtag != "") {
    chirp::Hashtags hashtag_proto;
    hashtag_proto.set_hashtag(hashtag);
    // chirp to string.
    chirp::Chirp *add_hashtag_chirp = hashtag_proto.add_chirps_with_hashtag();
    *add_hashtag_chirp = chirp;
  }

  return chirp;
}
// Constructs a FollowRequest and sends to service layer thru grpc and
// receives a FollowReply back
std::string ServiceLayer::follow(const std::string &username,
                                 const std::string &user_to_follow) {
  std::lock_guard<std::mutex> guard(mutex_);
  // check if user who is chirping exists in key value store
  std::string user_exists = backend_client_->Get(username).value();

  // user not found
  if (user_exists == "[empty_key]") {
    return "user_error";
  }
  // check for parent id's existance
  std::string to_follow_exists = backend_client_->Get(user_to_follow).value();
  if (to_follow_exists == "[empty_key]") {
    return "followee_error";
  }
  if (username == user_to_follow) {
    return "self_follow";
  }

  // Update user with new follower (to_follow), send to backend to update
  chirp::User user;
  user.ParseFromString(user_exists);
  std::vector<std::string> currently_following;
  std::copy(user.following().begin(), user.following().end(),
            std::back_inserter(currently_following));

  // don't follow someone more than once
  for (const std::string following : currently_following) {
    if (following == user_to_follow) {
      return "dup_follow";
    }
  }
  user.add_following(user_to_follow); // add as follower
  user.SerializeToString(&user_exists);
  backend_client_->Put(username, user_exists);
  return "success";
}
// Constructs a ReadRequest and sends to service layer thru grpc and receives
// a ReadReply back
std::vector<chirp::Chirp> ServiceLayer::read(const std::string &chirp_id) {
  std::vector<chirp::Chirp> chirp_thread;
  chirp::Chirp chirp;
  std::string chirp_serialized = backend_client_->Get(chirp_id).value();

  if (chirp_serialized.empty()) {
    return chirp_thread;
  }

  // Chirp was found, perform dfs to collect entire thread
  ThreadDFS(chirp_thread, chirp_id, 1);
}
void ServiceLayer::ThreadDFS(std::vector<chirp::Chirp> &chirp_thread,
                             std::string chirp_id, int depth) {
  // Get chirp from backend and get IDs of all children (replies)
  chirp::Chirp chirp;
  std::string chirp_serialized = backend_client_->Get(chirp_id).value();
  chirp.ParseFromString(chirp_serialized);

  std::vector<std::string> children_chirps_serialized;
  std::copy(chirp.child_ids().begin(), chirp.child_ids().end(),
            std::back_inserter(children_chirps_serialized));

  // if chirp hasn't been visited, add to reply and
  // set depth of reply - used for nested printing in client
  if (chirp.depth() == 0) {
    chirp.set_depth(depth);
    chirp_thread.push_back(chirp);
  }
  // chirp has no children recurse up -- base case
  if (children_chirps_serialized.empty()) {
    return;
  }

  // loop thru child chirps
  for (const std::string child_chirp : children_chirps_serialized) {
    ThreadDFS(chirp_thread, child_chirp, chirp.depth() + 1);
  }
}
// Constructs a MonitorRequest and sends to service layer thru grpc and
// receives a stream MonitorReply back
std::vector<chirp::Chirp>
ServiceLayer::monitor(const std::string &username,
                      std::vector<std::string> &followed_by_user,
                      bool registered) {

  std::vector<chirp::Chirp> new_chirps;

  // Register user as monitoring with everyone they follow
  if (!registered) {
    register_monitoring_user(username, followed_by_user);
  }
  // Update with  new chirps
  chirp::User user;
  std::string updated_user = backend_client_->Get(username).value();

  // Save new chirps arrived in user under "monitored_chirps" and then clear
  // them from user
  user.ParseFromString(updated_user);
  std::move(user.monitored_chirps().begin(), user.monitored_chirps().end(),
            std::back_inserter(new_chirps));

  if (new_chirps.size()) {
    user.clear_monitored_chirps();
    user.SerializeToString(&updated_user);
    backend_client_->Put(user.username(), updated_user);
  }
  return new_chirps;
}

std::optional<chirp::User>
ServiceLayer::validate_monitor_request(const std::string &username) {
  // check if user who is trying to monitor exists in key value store
  auto user_serialized = backend_client_->Get(username);
  if (user_serialized == "[empty_key]") {
    return {};
  }
  chirp::User user;
  user.ParseFromString(user_serialized.value());
  return user;
}

// Helper function for Monitor
void ServiceLayer::register_monitoring_user(
    const std::string &username, std::vector<std::string> &followed_by_user) {
  std::lock_guard<std::mutex> guard(mutex_);

  // Get all users the current uservalidate_monitor_request is following
  chirp::User user;
  user.ParseFromString(backend_client_->Get(username).value());
  std::copy(user.following().begin(), user.following().end(),
            std::back_inserter(followed_by_user));

  // register user as 'monitoring' w/ every person they are following
  for (const std::string followed : followed_by_user) {
    chirp::User followed_user;
    followed_user.ParseFromString(backend_client_->Get(followed).value());

    // Reject repeat follow attempts
    bool alreadyFollowing = false;
    for (const std::string currently_following :
         followed_user.followers_monitoring()) {
      if (currently_following == user.username()) {
        alreadyFollowing = true;
      }
    }
    // If not already a follower, add and save to backend
    if (!alreadyFollowing) {
      std::string *new_monitoring_follower =
          followed_user.add_followers_monitoring();
      *new_monitoring_follower = user.username();
      std::string followed_user_serialized;
      followed_user.SerializeToString(&followed_user_serialized);
      backend_client_->Put(followed, followed_user_serialized);
    }
  }
}
// Helper function for monitor
void ServiceLayer::terminate_monitor(chirp::User &user,
                                     std::vector<std::string> &followed_by_user,
                                     const std::string &username) {
  user.clear_monitored_chirps();
  // Remove user as currently monitoring from the people they follow
  for (const std::string followed_username : followed_by_user) {
    // Remove all monitoring followers
    chirp::User followed_user;
    followed_user.ParseFromString(backend_client_->Get(username).value());
    std::vector<std::string> followers_monitoring;
    std::copy(followed_user.followers_monitoring().begin(),
              followed_user.followers_monitoring().end(),
              std::back_inserter(followers_monitoring));
    followed_user.clear_followers_monitoring();

    // Add back all montoring followers except the current user (who
    // cancelled)
    for (const std::string current_follower : followers_monitoring) {
      if (username != current_follower) {
        std::string *current_monitoring_follower =
            followed_user.add_followers_monitoring();
        *current_monitoring_follower = current_follower;
      }
    }
    std::string users_monitoring;
    followed_user.SerializeToString(&users_monitoring);
    backend_client_->Put(
        followed_username,
        users_monitoring); // save back all users still monitoring
  }
}
std::vector<chirp::Chirp> ServiceLayer::stream(const std::string hashtag) {
  // TODO add hashtag to database. Create proto to save all the hashtags
  // messages!
  std::vector<chirp::Chirp> v;

  return v;
}
