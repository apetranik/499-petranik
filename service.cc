#include "service.h"

grpc::Status Service::registeruser(grpc::ServerContext *context,
                                   const chirp::RegisterRequest *request,
                                   chirp::RegisterReply *reply) {
  // check ifuser is already registered
  std::string user_serialized =
      backend_client_.SendGetRequest(request->username());
  if (!user_serialized.empty()) {
    return grpc::Status(grpc::StatusCode::ALREADY_EXISTS,
                        "User Already Exists");
  }

  // User doesn't exist, so save to backend
  chirp::User user;
  user.set_username(request->username());
  user.SerializeToString(&user_serialized);
  backend_client_.SendPutRequest(request->username(), user_serialized);
  return grpc::Status::OK;
}

grpc::Status Service::chirp(grpc::ServerContext *context,
                            const chirp::ChirpRequest *request,
                            chirp::ChirpReply *reply) {
  std::lock_guard<std::mutex> guard(mutex_);  // get lock

  // check if user who is chirping exists in key value store
  std::string user_exists = backend_client_.SendGetRequest(request->username());
  if (user_exists.empty()) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "User not found");
  }

  // check for parent id's existance
  if (!request->parent_id().empty()) {
    std::string parent_exists =
        backend_client_.SendGetRequest(request->parent_id());
    if (parent_exists.empty()) {
      return grpc::Status(grpc::StatusCode::NOT_FOUND, "Parent ID not found");
    }
  }

  // User does exist, so allow them to chirp
  // Create new chirp ID based on current number of chirps
  std::string num_chirps_str = backend_client_.SendGetRequest("num_chirps");
  int num_chirps = std::stoi(num_chirps_str);
  ++num_chirps;
  num_chirps_str = std::to_string(num_chirps);
  backend_client_.SendPutRequest("num_chirps",
                                 num_chirps_str);  // update new chirp count

  // construct new Chirp and fill with relevant params
  chirp::Chirp *chirp = reply->mutable_chirp();
  chirp->set_username(request->username());
  chirp->set_text(request->text());
  chirp->set_id(num_chirps_str);

  if (!request->parent_id().empty()) {
    chirp->set_parent_id(request->parent_id());
  }

  // generate time stamp
  chirp::Timestamp *stamp = new chirp::Timestamp();
  int64_t seconds = google::protobuf::util::TimeUtil::TimestampToSeconds(
      google::protobuf::util::TimeUtil::GetCurrentTime());
  int64_t useconds = google::protobuf::util::TimeUtil::TimestampToMicroseconds(
      google::protobuf::util::TimeUtil::GetCurrentTime());
  stamp->set_seconds(seconds);
  stamp->set_useconds(useconds);
  chirp->set_allocated_timestamp(stamp);

  std::string chirp_serialized;
  chirp->SerializeToString(&chirp_serialized);  // Serialize chirp to string
  backend_client_.SendPutRequest(chirp->id(),
                                 chirp_serialized);  // add new chirp to backend

  // If applicable, update Parent Chirp with new child reply
  if (!request->parent_id().empty()) {
    std::string parent_chirp_serialized =
        backend_client_.SendGetRequest(request->parent_id());
    chirp::Chirp parent_chirp;
    parent_chirp.ParseFromString(parent_chirp_serialized);
    std::string *new_child_reply_id = parent_chirp.add_child_ids();
    *new_child_reply_id = chirp->id();
    parent_chirp.SerializeToString(&parent_chirp_serialized);
    backend_client_.SendPutRequest(request->parent_id(),
                                   parent_chirp_serialized);
  }

  // Check for monitoring users, if any, send new chirp to monitoring followers
  chirp::User user;
  std::string user_serialized =
      backend_client_.SendGetRequest(request->username());
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
        backend_client_.SendGetRequest(username);  // Get follower
    monitoring_user.ParseFromString(monitoring_user_serialized);
    chirp::Chirp *incoming_chirp =
        monitoring_user
            .add_monitored_chirps();  // add new chirp to be monitored
    *incoming_chirp = *chirp;         // fill chirp
    monitoring_user.SerializeToString(&monitoring_user_serialized);
    backend_client_.SendPutRequest(
        username, monitoring_user_serialized);  // save chirp to follower
  }

  return grpc::Status::OK;
}

grpc::Status Service::follow(grpc::ServerContext *context,
                             const chirp::FollowRequest *request,
                             chirp::FollowReply *reply) {
  std::lock_guard<std::mutex> guard(mutex_);

  // check if user who is chirping exists in key value store
  std::string user_exists = backend_client_.SendGetRequest(request->username());

  if (user_exists.empty()) {
    std::cout << "why are we here" << std::endl;
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "User not found");
  }

  // check for parent id's existance
  std::string to_follow_exists =
      backend_client_.SendGetRequest(request->to_follow());
  if (to_follow_exists.empty()) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND,
                        "Requested user to follow not found");
  }

  // Update user with new follower (to_follow), send to backend to update
  chirp::User user;
  user.ParseFromString(user_exists);
  std::vector<std::string> currently_following;
  std::copy(user.following().begin(), user.following().end(),
            std::back_inserter(currently_following));

  // don't follow someone more than once
  for (const std::string following : currently_following) {
    if (following == request->to_follow()) {
      return grpc::Status(grpc::StatusCode::ALREADY_EXISTS,
                          "Already following");
    }
  }
  user.add_following(request->to_follow());  // add as follower
  user.SerializeToString(&user_exists);
  backend_client_.SendPutRequest(request->username(), user_exists);
  return grpc::Status::OK;
}

void Service::ThreadDFS(chirp::ReadReply *reply, std::string chirp_id,
                        int depth) {
  // Get chirp from backend and get IDs of all children (replies)
  chirp::Chirp chirp;
  std::string chirp_serialized = backend_client_.SendGetRequest(chirp_id);
  chirp.ParseFromString(chirp_serialized);

  std::vector<std::string> children_chirps_serialized;
  std::copy(chirp.child_ids().begin(), chirp.child_ids().end(),
            std::back_inserter(children_chirps_serialized));

  // if chirp hasn't been visited, add to reply and
  // set depth of reply - used for nested printing in client
  if (chirp.depth() == 0) {
    chirp.set_depth(depth);
    chirp::Chirp *new_chirp = reply->add_chirps();
    *new_chirp = chirp;
  }
  // chirp has no children recurse up -- base case
  if (children_chirps_serialized.empty()) {
    return;
  }

  // loop thru child chirps
  for (const std::string child_chirp : children_chirps_serialized) {
    ThreadDFS(reply, child_chirp, chirp.depth() + 1);
  }
}

grpc::Status Service::read(grpc::ServerContext *context,
                           const chirp::ReadRequest *request,
                           chirp::ReadReply *reply) {
  // Check if chirp ID exists in key value store
  chirp::Chirp chirp;
  std::string chirp_serialized =
      backend_client_.SendGetRequest(request->chirp_id());

  if (chirp_serialized.empty()) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "Chirp not found");
  }

  // Chirp was found, perform dfs to collect entire thread
  std::vector<chirp::Chirp> *chirp_thread;
  ThreadDFS(reply, request->chirp_id(), 1);
  return grpc::Status::OK;
}
// Helper function for Monitor
void Service::register_monitoring_user(
    chirp::User &user, std::vector<std::string> &followed_by_user,
    const std::string &username) {
  std::lock_guard<std::mutex> guard(mutex_);

  // Get all users the current user is following
  user.ParseFromString(backend_client_.SendGetRequest(username));
  std::copy(user.following().begin(), user.following().end(),
            std::back_inserter(followed_by_user));

  // register user as 'monitoring' w/ every person they are following
  for (const std::string followed : followed_by_user) {
    chirp::User followed_user;
    followed_user.ParseFromString(backend_client_.SendGetRequest(followed));

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
      backend_client_.SendPutRequest(followed, followed_user_serialized);
    }
  }
}
// Helper function for monitor
void Service::terminate_monitor(chirp::User &user,
                                std::vector<std::string> &followed_by_user,
                                const std::string &username) {
  user.clear_monitored_chirps();
  // Remove user as currently monitoring from the people they follow
  for (const std::string followed_username : followed_by_user) {
    // Remove all monitoring followers
    chirp::User followed_user;
    followed_user.ParseFromString(backend_client_.SendGetRequest(username));
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
    backend_client_.SendPutRequest(
        followed_username,
        users_monitoring);  // save back all users still monitoring
  }
}
// Helper function for monitor
void Service::stream_new_chirps(chirp::User &user,
                                grpc::ServerWriter<chirp::MonitorReply> *stream,
                                chirp::MonitorReply &reply) {
  std::vector<chirp::Chirp> new_chirps;  // filled with new chirps
  std::move(user.monitored_chirps().begin(), user.monitored_chirps().end(),
            std::back_inserter(new_chirps));

  // get all new chirps since last check and write to stream
  for (const chirp::Chirp chirp : new_chirps) {
    chirp::Chirp *c = reply.mutable_chirp();
    *c = chirp;
    stream->Write(reply);
  }
}
grpc::Status Service::monitor(grpc::ServerContext *context,
                              const chirp::MonitorRequest *request,
                              grpc::ServerWriter<chirp::MonitorReply> *stream) {
  chirp::MonitorReply reply;
  chirp::User user;                           // User who is monitoring
  std::vector<std::string> followed_by_user;  // List of users followed by user

  // Register user as monitoring with everyone they follow
  register_monitoring_user(user, followed_by_user, request->username());

  // Continously monitor for new chirps
  std::string updated_user;  // updated w/ new incoming chirps
  std::string current_user;  // current serialized state of user
  while (true) {
    // Execute interrupted w/ ^c, clear all cached chirps & remove user as
    // monitoring from all people they follow
    if (context->IsCancelled()) {
      terminate_monitor(user, followed_by_user, request->username());
      return grpc::Status::CANCELLED;
    }

    // Continue to monitor
    updated_user = backend_client_.SendGetRequest(request->username());

    // New chirp arrived in user's monitored chirps
    if (updated_user != current_user) {
      user.ParseFromString(updated_user);
      stream_new_chirps(user, stream, reply);  // write to stream
      // Now that new chirps have been written to stream, clear from user
      user.clear_monitored_chirps();
      user.SerializeToString(&current_user);
      backend_client_.SendPutRequest(user.username(), current_user);
    }
  }
  return grpc::Status::OK;
}

grpc::Status Service::validate_monitor_request(
    grpc::ServerContext *context, const chirp::MonitorRequest *request,
    chirp::Followers *reply) {
  // check if user who is trying to monitor exists in key value store
  std::string user_exists = backend_client_.SendGetRequest(request->username());
  if (user_exists.empty()) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "User not found");
  }

  chirp::User user;
  user.ParseFromString(user_exists);

  // User is not following anyone, not necessarily an error, but good to let
  // them know
  if (user.following().empty()) {
    return grpc::Status(grpc::StatusCode::UNIMPLEMENTED,
                        "Not following anyone");
  }

  // Tell user who they are currently following
  if (!user.following().empty()) {
    std::vector<std::string> following_users;  // followers_monitoring
    std::copy(user.following().begin(), user.following().end(),
              std::back_inserter(following_users));
    // construct msg with current users you are followering
    std::string msg;
    for (const std::string user : following_users) {
      msg += "'";
      msg += user;
      msg += "'";
      msg += ", ";
    }
    reply->set_names(msg);
    return grpc::Status::OK;
  }
  return grpc::Status::OK;
}
