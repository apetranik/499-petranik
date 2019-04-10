#include "service_controller.h"

grpc::Status
ServiceController::registeruser(grpc::ServerContext *context,
                                const chirp::RegisterRequest *request,
                                chirp::RegisterReply *reply) {
  // check if user is already registered
  std::cout << "here2" << std::endl;
  bool success = service_.registeruser(request->username());
  if (!success) {
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                        "Usernames must be unique");
  }
  return grpc::Status::OK;
}

grpc::Status ServiceController::chirp(grpc::ServerContext *context,
                                      const chirp::ChirpRequest *request,
                                      chirp::ChirpReply *reply) {
  auto chirp = service_.chirp(request->username(), request->text(),
                              request->parent_id());
  // Nothing returned, user doesn't exist or parent id doesn't exist
  if (!chirp) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND,
                        "Verify correct user and/or parent ID");
  }
  chirp::Chirp *new_chirp = reply->mutable_chirp();
  *new_chirp = chirp.value();
  return grpc::Status::OK;
}

grpc::Status ServiceController::follow(grpc::ServerContext *context,
                                       const chirp::FollowRequest *request,
                                       chirp::FollowReply *reply) {
  std::string status_msg =
      service_.follow(request->username(), request->to_follow());
  if (status_msg == "user_error") {
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "User not found");
  }
  if (status_msg == "followee_error") {
    return grpc::Status(grpc::StatusCode::NOT_FOUND,
                        "Requested User to follow not found");
  }
  if (status_msg == "self_follow") {
    return grpc::Status(grpc::StatusCode::ALREADY_EXISTS,
                        "Cannot follow yourself");
  }
  if (status_msg == "dup_follow") {
    return grpc::Status(grpc::StatusCode::ALREADY_EXISTS,
                        "Duplicate follow request");
  }
  if (status_msg == "success") {
    return grpc::Status::OK;
  }
}

grpc::Status ServiceController::read(grpc::ServerContext *context,
                                     const chirp::ReadRequest *request,
                                     chirp::ReadReply *reply) {
  std::vector<chirp::Chirp> chirp_thread = service_.read(request->chirp_id());

  // Check if chirp ID exists in key value store
  if (!chirp_thread.size()) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "Chirp not found");
  }
  // add chirps to reply
  for (const auto chirp : chirp_thread) {
    chirp::Chirp *new_chirp = reply->add_chirps();
    *new_chirp = chirp;
  }
  return grpc::Status::OK;
}
// Helper function
grpc::Status ServiceController::validate_monitor_request(
    grpc::ServerContext *context, const chirp::MonitorRequest *request,
    chirp::Followers *reply) {
  // check if user who is trying to monitor exists in key value store
  auto usr = service_.validate_monitor_request(request->username());
  if (!usr) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "User not found");
  }
  chirp::User user = usr.value();

  if (user.following().empty()) {
    return grpc::Status(grpc::StatusCode::UNIMPLEMENTED,
                        "Not following anyone");
  }

  // Tell user who they are currently following
  if (!user.following().empty()) {
    std::vector<std::string> following_users; // followers_monitoring
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

grpc::Status
ServiceController::monitor(grpc::ServerContext *context,
                           const chirp::MonitorRequest *request,
                           grpc::ServerWriter<chirp::MonitorReply> *stream) {
  std::cout << "check monitor 2" << std::endl;
  chirp::MonitorReply reply;
  chirp::User user;
  std::vector<std::string> followed_by_user;

  bool registeredAsMonitoring = false;
  // poll for new chirps
  while (true) {
    // Execute interrupted w/ ^c, clear all cached chirps & remove user as
    // monitoring from all people they follow
    if (context->IsCancelled()) {
      service_.terminate_monitor(user, followed_by_user, request->username());
      return grpc::Status::CANCELLED;
    }
    std::vector<chirp::Chirp> chirp_stream = service_.monitor(
        request->username(), followed_by_user, registeredAsMonitoring);

    // get all new chirps since last check and write to stream
    for (const chirp::Chirp chirp : chirp_stream) {
      chirp::Chirp *c = reply.mutable_chirp();
      *c = chirp;
      stream->Write(reply);
    }
    registeredAsMonitoring = true;
  }
  return grpc::Status::OK;
}

grpc::Status
ServiceController::stream(grpc::ServerContext *context,
                          const chirp::StreamRequest *request,
                          grpc::ServerWriter<chirp::StreamReply> *stream) {
  chirp::StreamReply reply;
  chirp::Chirp chirp;
  // TODO call stream in service.h
  return grpc::Status::OK;
}
