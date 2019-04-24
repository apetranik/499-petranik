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

// Constructs a ChirpRequest and sends to service layer thru grpc and receives
// a ChirpReply back
std::optional<chirp::Chirp> ServiceLayer::chirp(const std::string &username,
                                                const std::string &text,
                                                const std::string &parent_id) {
  std::lock_guard<std::mutex> guard(mutex_);  // get lock

  std::vector<std::string> hashtags = CheckIfHaveMultipleHashtags(text);
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
  bool success =
      backend_client_->Put("num_chirps",
                           num_chirps_str);  // update new chirp count

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
  chirp.SerializeToString(&chirp_serialized);  // Serialize chirp to string
  backend_client_->Put(chirp.id(),
                       chirp_serialized);  // add new chirp to backend

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

  // Check for monitoring users, if any, send new chirp to monitoring
  // followers
  chirp::User user;
  std::string user_serialized = backend_client_->Get(username).value();
  user.ParseFromString(user_serialized);

  std::vector<std::string> monitoring_users_serialized;  // followers_monitoring
  std::vector<chirp::User> monitoring_users;
  std::copy(user.followers_monitoring().begin(),
            user.followers_monitoring().end(),
            std::back_inserter(monitoring_users_serialized));

  std::string monitoring_user_serialized;
  chirp::User monitoring_user;

  // iterate thru all currently monitoring users and give them new chirp
  for (const std::string username : monitoring_users_serialized) {
    monitoring_user_serialized =
        backend_client_->Get(username).value();  // Get follower
    monitoring_user.ParseFromString(monitoring_user_serialized);
    chirp::Chirp *incoming_chirp =
        monitoring_user
            .add_monitored_chirps();  // add new chirp to be monitored
    *incoming_chirp = chirp;          // fill chirp
    monitoring_user.SerializeToString(&monitoring_user_serialized);
    backend_client_->Put(username,
                         monitoring_user_serialized);  // save chirp to follower
  }

  // add to hashtag if exist, not empty
  if (hashtags.size() > 0) {
    for (int i = 0; i < hashtags.size(); i++) {
      AddHashtagToDatabase(chirp, hashtags[i]);
    }
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
  user.add_following(user_to_follow);  // add as follower
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
std::vector<chirp::Chirp> ServiceLayer::monitor(
    const std::string &username, std::vector<std::string> &followed_by_user,
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

std::optional<chirp::User> ServiceLayer::validate_monitor_request(
    const std::string &username) {
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
        users_monitoring);  // save back all users still monitoring
  }
}
std::vector<chirp::Chirp> ServiceLayer::stream(
    const std::string hashtag, std::time_t seconds,
    int64_t microseconds_since_epoch, std::set<std::string> &chirp_sent) {
  std::vector<chirp::Chirp> to_send;
  auto from_get = backend_client_->Get("[" + hashtag + "]");

  if (from_get.value() != kEmptyKey) {
    chirp::Hashtags get_hashtags;
    get_hashtags.ParseFromString(from_get.value());

    for (int i = 0; i < get_hashtags.chirps_with_hashtag_size(); i++) {
      // if the chirps with this hashtag is chirped after the current
      // timestamp
      auto it = chirp_sent.find(get_hashtags.chirps_with_hashtag(i).id());
      if (get_hashtags.chirps_with_hashtag(i).timestamp().useconds() >=
              microseconds_since_epoch &&
          (it == chirp_sent.end())) {
        chirp::Chirp chirp_copy;
        CopyChirp(&chirp_copy, get_hashtags.chirps_with_hashtag(i));
        to_send.push_back(chirp_copy);
        chirp_sent.insert(chirp_copy.id());
      }
    }
  }
  return to_send;
}
std::vector<chirp::Chirp> ServiceLayer::TestStream(const std::string hashtag) {
  std::time_t seconds;
  int64_t microseconds_since_epoch;
  chirp::StreamReply reply;
  chirp::Chirp chirp;
  std::vector<chirp::Chirp> to_send_back;
  int chirps_check_left = 10;
  SetTimeStamp(seconds, microseconds_since_epoch);
  std::set<std::string> chirp_sent;
  while (chirps_check_left > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // Calls the stream function from servicelayer to check for new chirps with
    // hashtags.
    std::vector<chirp::Chirp> chirp_stream;
    chirp_stream =
        stream(hashtag, seconds, microseconds_since_epoch, chirp_sent);
    for (const chirp::Chirp chirp : chirp_stream) {
      to_send_back.push_back(chirp);
    }
    chirps_check_left--;
  }
  return to_send_back;
}
void ServiceLayer::CopyChirp(chirp::Chirp *empty_chirp,
                             chirp::Chirp chirp_to_copy) {
  empty_chirp->set_username(chirp_to_copy.username());
  empty_chirp->set_text(chirp_to_copy.text());
  empty_chirp->set_id(chirp_to_copy.id());
  empty_chirp->set_parent_id(chirp_to_copy.parent_id());
  chirp::Timestamp *time = empty_chirp->mutable_timestamp();
  time->set_seconds(chirp_to_copy.timestamp().seconds());
  time->set_useconds(chirp_to_copy.timestamp().useconds());
  for (int j = 0; j < chirp_to_copy.child_ids_size(); j++) {
    auto copy_child_ids = empty_chirp->add_child_ids();
    *copy_child_ids = chirp_to_copy.child_ids(j);
  }
  empty_chirp->set_depth(chirp_to_copy.depth());
}
void ServiceLayer::SetTimeStamp(std::time_t &seconds,
                                int64_t &microseconds_since_epoch) {
  seconds = std::time(nullptr);
  microseconds_since_epoch =
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();
}
std::string ServiceLayer::CheckIfHaveHashtag(const std::string &text) {
  std::size_t found = text.find('#');
  std::string hashtagword = "";
  if (found != std::string::npos) {
    for (int i = found; i < text.length(); i++) {
      if (text[i] == ' ') {
        break;
      } else {
        hashtagword += text[i];
      }
    }
    hashtagword = trim(hashtagword);
    return hashtagword;
  }
}
std::vector<std::string> ServiceLayer::CheckIfHaveMultipleHashtags(
    const std::string &text) {
  std::vector<std::string> to_send;
  std::string edit_text = text;
  std::size_t found = edit_text.find('#');

  while (edit_text != "" && (found != std::string::npos)) {
    found = edit_text.find('#');
    if (found == std::string::npos) {
      return to_send;
    }
    std::string hashtag = CheckIfHaveHashtag(edit_text);
    if (hashtag != "") {
      to_send.push_back(hashtag);
      size_t pos = edit_text.find(hashtag);
      if (pos != std::string::npos) {
        // If found then erase it from edit_text
        edit_text.erase(pos, hashtag.length());
      }
    }
  }
  return to_send;
}
std::string ServiceLayer::trim(std::string &str) {
  size_t first = str.find_first_not_of(' ');
  if (first == std::string::npos) {
    return "";
  }
  size_t last = str.find_last_not_of(' ');
  return str.substr(first, (last - first + 1));
}
void ServiceLayer::AddHashtagToDatabase(const chirp::Chirp &chirp,
                                        const std::string &hashtagword) {
  // Figure out if this hashtag word exist in database already, if so, just
  // append to that proto, if not make a new hashtag proto
  std::string offical_hashtag = "[" + hashtagword + "]";
  auto from_get = backend_client_->Get(offical_hashtag);
  if (from_get.value() != kEmptyKey) {
    chirp::Hashtags create_hashtag;
    create_hashtag.ParseFromString(from_get.value());
    chirp::Chirp *adding_chirp_with_hashtag =
        create_hashtag.add_chirps_with_hashtag();
    *adding_chirp_with_hashtag = chirp;

    std::string hashtag_tostring;
    create_hashtag.SerializeToString(&hashtag_tostring);
    backend_client_->Put(offical_hashtag, hashtag_tostring);
  } else {
    chirp::Hashtags hashtag_proto;
    hashtag_proto.set_hashtag(offical_hashtag);
    chirp::Chirp *add_hashtag_chirp = hashtag_proto.add_chirps_with_hashtag();
    *add_hashtag_chirp = chirp;

    std::string hashtag_tostring;
    hashtag_proto.SerializeToString(&hashtag_tostring);
    backend_client_->Put(offical_hashtag, hashtag_tostring);
  }
}
