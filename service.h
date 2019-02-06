#include <memory>
#include <string>
#include <vector>

#include <grpcpp/channel.h>
#include "service.grpc.pb.h"

// A backend service to receive requests from client (command line)
class Service final : public chirp::ServiceLayer::Service {
 public:
  Service() {}
  
  // Registers the given non-blank username
  grpc::Status registeruser(grpc::ServerContext *context, const chirp::RegisterRequest *request, chirp::RegisterReply *reply);

  // Posts a new chirp (optionally as a reply)
  grpc::Status chirp(grpc::ServerContext *context, const chirp::ChirpRequest *request, chirp::ChirpReply *reply);
  
  // Starts following a given user
  grpc::Status follow(grpc::ServerContext *context, const chirp::FollowRequest *request, chirp::FollowReply *reply);

  // Reads a chirp thread from the given id
  grpc::Status read(grpc::ServerContext *context, const chirp::ReadRequest *request, chirp::ReadReply *reply);

  // Streams chirps from all followed users
  grpc::Status monitor(grpc::ServerContext *context, const chirp::MonitorRequest *request, chirp::MonitorReply *reply);

};


