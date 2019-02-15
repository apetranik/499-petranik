#include "service.h"

#include <chrono>
#include <thread>
#include <mutex>
#include <vector>

#include <google/protobuf/util/time_util.h>
#include <google/protobuf/repeated_field.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>

#include "service.grpc.pb.h"
#include "data_storage_types.grpc.pb.h"

grpc::Status Service::registeruser(grpc::ServerContext *context, const chirp::RegisterRequest *request, chirp::RegisterReply *reply) {
  // check ifuser is already registered
  std::string user_serialized = backend_client_.SendGetRequest(request->username());
  if(!user_serialized.empty()) {
    return grpc::Status(grpc::StatusCode::ALREADY_EXISTS, "User Already Exists");
  }

  // User doesn't exist, so save to backend
  chirp::User user;
  user.set_username(request->username());
  user.SerializeToString(&user_serialized); 
  backend_client_.SendPutRequest(request->username(), user_serialized);
  return grpc::Status::OK;
}

grpc::Status Service::chirp(grpc::ServerContext *context, const chirp::ChirpRequest *request, chirp::ChirpReply *reply) {
  std::lock_guard<std::mutex> guard(mutex_); // get lock

  // check if user who is chirping exists in key value store
  std::string user_exists = backend_client_.SendGetRequest(request->username());
  if(user_exists.empty()) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "User not found");
  }

  // check for parent id's existance
  if(!request->parent_id().empty()) {
    std::string parent_exists = backend_client_.SendGetRequest(request->parent_id());
    if(parent_exists.empty()) {
      return grpc::Status(grpc::StatusCode::NOT_FOUND, "Parent ID not found");
    }
  }

  // User does exist, so allow them to chirp
  // Create new chirp ID based on current number of chirps
  std::string num_chirps_str = backend_client_.SendGetRequest("num_chirps");
  int num_chirps;
  if(num_chirps_str.empty()) {
    num_chirps = 1; // first chirp in system
  }
  else {
    num_chirps = std::stoi(num_chirps_str);
    ++num_chirps;
  }
  num_chirps_str = std::to_string(num_chirps);
  backend_client_.SendPutRequest("num_chirps", num_chirps_str); // update new chirp count

  // construct new Chirp and fill with relevant params
  chirp::Chirp *chirp = reply->mutable_chirp();
  chirp->set_username(request->username());
  chirp->set_text(request->text());
  chirp->set_id(num_chirps_str);

  if(!request->parent_id().empty()) {
    chirp->set_parent_id(request->parent_id());
  }

  // generate time stamp
  chirp::Timestamp *stamp = new chirp::Timestamp();
  int64_t seconds = google::protobuf::util::TimeUtil::TimestampToSeconds(google::protobuf::util::TimeUtil::GetCurrentTime());
  int64_t useconds = google::protobuf::util::TimeUtil::TimestampToMicroseconds(google::protobuf::util::TimeUtil::GetCurrentTime());
  stamp->set_seconds(seconds);
  stamp->set_useconds(useconds);
  chirp->set_allocated_timestamp(stamp);

  std::string chirp_serialized;
  chirp->SerializeToString(&chirp_serialized); // Serialize chirp to string
  backend_client_.SendPutRequest(chirp->id(), chirp_serialized); // add new chirp to backend

  // If applicable, update Parent Chirp with new child reply
  if(!request->parent_id().empty()) {
    std::string parent_chirp_serialized = backend_client_.SendGetRequest(request->parent_id());
    chirp::Chirp parent_chirp;
    parent_chirp.ParseFromString(parent_chirp_serialized);
    std::string *new_child_reply_id = parent_chirp.add_child_ids();
    *new_child_reply_id = chirp->id();
    parent_chirp.SerializeToString(&parent_chirp_serialized);
    backend_client_.SendPutRequest(request->parent_id(), parent_chirp_serialized);
  }

  // Check for monitoring users, if any, send new chirp to monitoring followers
  chirp::User user;
  std::string user_serialized = backend_client_.SendGetRequest(request->username());
  user.ParseFromString(user_serialized);

  std::vector<std::string> monitoring_users_serialized; // followers_monitoring
  std::vector<chirp::User> monitoring_users;
  std::copy(user.followers_monitoring().begin(), user.followers_monitoring().end(), std::back_inserter(monitoring_users_serialized));
  
  std::string monitoring_user_serialized;
  chirp::User monitoring_user;

  // iterate thru all currently monitoring users and give them new chirp
  for(std::string username : monitoring_users_serialized) {
    monitoring_user_serialized = backend_client_.SendGetRequest(username); // Get follower
    monitoring_user.ParseFromString(monitoring_user_serialized);
    chirp::Chirp *incoming_chirp = monitoring_user.add_monitored_chirps(); // add new chirp to be monitored
    *incoming_chirp = *chirp; // fill chirp
    monitoring_user.SerializeToString(&monitoring_user_serialized); 
    backend_client_.SendPutRequest(username, monitoring_user_serialized); // save chirp to follower
  }

  return grpc::Status::OK;
}

grpc::Status Service::follow(grpc::ServerContext *context, const chirp::FollowRequest *request, chirp::FollowReply *reply) {
  std::lock_guard<std::mutex> guard(mutex_);

  // check if user who is chirping exists in key value store
  std::string user_exists = backend_client_.SendGetRequest(request->username());

  if(user_exists.empty()) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "User not found");
  }

  // check for parent id's existance
  std::string to_follow_exists = backend_client_.SendGetRequest(request->to_follow());
  if(to_follow_exists.empty()) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "Requested user to follow not found");
  }

  // Update user with new follower (to_follow), send to backend to update
  chirp::User user;
  user.ParseFromString(user_exists);
  std::vector<std::string> currently_following;
  std::copy(user.following().begin(), user.following().end(), std::back_inserter(currently_following));

  // don't follow someone more than once
  for(std::string following : currently_following) {
    if(following == request->to_follow()) {
      return grpc::Status(grpc::StatusCode::ALREADY_EXISTS, "Already following");
    }
  }
  user.add_following(request->to_follow()); // add as follower
  user.SerializeToString(&user_exists); 
  backend_client_.SendPutRequest(request->username(), user_exists);

  return grpc::Status::OK;
}

void Service::ThreadDFS(chirp::ReadReply *reply, std::string chirp_id, int depth)
{
  // Get chirp from backend and get IDs of all children (replies)
  chirp::Chirp chirp;
  std::string chirp_serialized = backend_client_.SendGetRequest(chirp_id);
  chirp.ParseFromString(chirp_serialized);

  std::vector<std::string> children_chirps_serialized; 
  std::copy(chirp.child_ids().begin(), chirp.child_ids().end(), std::back_inserter(children_chirps_serialized));

  // if chirp hasn't been visited, add to reply and 
  // set depth of reply - used for nested printing in client
  if(chirp.depth() == 0) {
      chirp.set_depth(depth);
      chirp::Chirp *new_chirp = reply->add_chirps();
      *new_chirp = chirp;
  }
  // chirp has no children recurse up -- base case
  if(children_chirps_serialized.empty()) {
    return;
  }
  
  // loop thru child chirps
  for(std::string child_chirp : children_chirps_serialized) {
    ThreadDFS(reply, child_chirp, chirp.depth()+1); 
  }
}

grpc::Status Service::read(grpc::ServerContext *context, const chirp::ReadRequest *request, chirp::ReadReply *reply) {
  // Check if chirp ID exists in key value store
  chirp::Chirp chirp;
  std::string chirp_serialized = backend_client_.SendGetRequest(request->chirp_id());

  if(chirp_serialized.empty()) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "Chirp not found");
  }

  // Chirp was found, perform dfs to collect entire thread
  std::vector<chirp::Chirp> *chirp_thread;
  ThreadDFS(reply, request->chirp_id(), 1);
  return grpc::Status::OK;
}

grpc::Status Service::monitor(grpc::ServerContext *context, const chirp::MonitorRequest *request, grpc::ServerWriter<chirp::MonitorReply> *stream) {
  chirp::MonitorReply reply;

  std::vector<std::string> following; // all people user is currently following
  std::string followed_user_serialized;
  chirp::User user; // user who requested to monitor

  {
    std::lock_guard<std::mutex> guard(mutex_); // acquire lock

    std::string current_user = backend_client_.SendGetRequest(request->username());
    user.ParseFromString(current_user);
    // Get all users the current user is following
    std::copy(user.following().begin(), user.following().end(), std::back_inserter(following));
    std::vector<chirp::User> monitoring_users;

    // register user as 'monitoring' w/ every person they are following
    for(std::string followed : following) {
      chirp::User followed_user;
      followed_user_serialized = backend_client_.SendGetRequest(followed);
      followed_user.ParseFromString(followed_user_serialized);

      // Reject repeat follow attempts
      bool alreadyFollowing = false;
      for(std::string currently_following : followed_user.followers_monitoring()) {
        if(currently_following == user.username()) {
          alreadyFollowing = true;
        }
      }
      // If not already a follower, add and save to backend
      if(!alreadyFollowing) {
        std::string *new_monitoring_follower = followed_user.add_followers_monitoring();
        *new_monitoring_follower = user.username();
        followed_user.SerializeToString(&followed_user_serialized);       
        backend_client_.SendPutRequest(followed, followed_user_serialized); 
      }
    }
  }

  // Continously monitor for new chirps
  std::string updated_user; // used to make sure we don't keep printing the same new chirps over and over
  std::string current_user;
  std::vector<chirp::Chirp> monitored_chirps;

  while(true) {
    // If user pressed ^c, remove user as monitoring from all users followed and clear all current cached chirps
    if(context->IsCancelled()) {
      user.clear_monitored_chirps();
      user.SerializeToString(&updated_user);

      // remove self as currently monitoring
      for(std::string followed : following) {
        chirp::User followed_user;
        followed_user_serialized = backend_client_.SendGetRequest(followed);
        followed_user.ParseFromString(followed_user_serialized);

        std::vector<std::string> followers_monitoring;
        std::copy(followed_user.followers_monitoring().begin(), followed_user.followers_monitoring().end(), std::back_inserter(followers_monitoring));
        followed_user.clear_followers_monitoring();

        // add back all monitoring followers for a user being monitored, except the current user who just cancelled the monitor
        for(std::string current_follower : followers_monitoring) {
          if(request->username() != current_follower) {
            std::string *current_monitoring_follower = followed_user.add_followers_monitoring();
            *current_monitoring_follower = current_follower;
          }
        }

        followed_user.SerializeToString(&followed_user_serialized); 
        backend_client_.SendPutRequest(followed, followed_user_serialized); // save user as currently monitoring follower
      }
      return grpc::Status::CANCELLED;
    }
    // Else user did not cancel, so continue monitor
    updated_user = backend_client_.SendGetRequest(request->username());
    user.ParseFromString(updated_user);
    
    // new chirp from follower has occured since last iteration
    if(updated_user != current_user) {
      monitored_chirps.clear();
      std::move(user.monitored_chirps().begin(), user.monitored_chirps().end(), std::back_inserter(monitored_chirps));

      // get all new chirps and write to stream
      for(chirp::Chirp chirp : monitored_chirps) {
        chirp::Chirp* c = reply.mutable_chirp();
        *c = chirp;
        stream->Write(reply);
      }
      user.clear_monitored_chirps();
      user.SerializeToString(&current_user);
      backend_client_.SendPutRequest(user.username(), current_user); // make sure to clear chirps so we don't print more than once
    } 
  }

  return grpc::Status::OK;
}


grpc::Status Service::monitor_check(grpc::ServerContext *context, const chirp::MonitorRequest *request, chirp::Followers* reply) {
  // check if user who is trying to monitor exists in key value store
  std::string user_exists = backend_client_.SendGetRequest(request->username());
  if(user_exists.empty()) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "User not found");
  }

  chirp::User user;
  user.ParseFromString(user_exists);

  // User is not following anyone, not necessarily an error, but good to let them know
  if(user.following().empty()) {
    return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "not following anyone");
  }

  // Tell user who they are currently following
  if(!user.following().empty()) {
    std::vector<std::string> following_users; //followers_monitoring
    std::copy(user.following().begin(), user.following().end(), std::back_inserter(following_users));
    // construct msg with current users you are followering
    std::string msg;
    for(std::string user : following_users) {
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

// Sets up GRPC and starts server
void run_server() {
  std::string server_address("0.0.0.0:50000");
  Service service;
  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server is listening on " << server_address << std::endl;

  server->Wait();
}

int main(int argc, char **argv) {
  run_server();
  return 0;
}
