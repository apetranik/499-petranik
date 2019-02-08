#include <memory>
#include <string>
#include <vector>

#include <grpcpp/channel.h>
#include "backend_client.h"
#include "service.grpc.pb.h"

// A backend service to receive requests from client (command line)
class Service final : public chirp::ServiceLayer::Service {
 public:
   Service() : backend_client(grpc::CreateChannel("localhost:50002", grpc::InsecureChannelCredentials())) {}
  
   // Registers the given non-blank username
   grpc::Status registeruser(grpc::ServerContext *context, const chirp::RegisterRequest *request, chirp::RegisterReply *reply);

   // Posts a new chirp (optionally as a reply)
   grpc::Status chirp(grpc::ServerContext *context, const chirp::ChirpRequest *request, chirp::ChirpReply *reply);
  
   // Starts following a given user
   grpc::Status follow(grpc::ServerContext *context, const chirp::FollowRequest *request, chirp::FollowReply *reply);

   // Reads a chirp thread from the given id
   grpc::Status read(grpc::ServerContext *context, const chirp::ReadRequest *request, chirp::ReadReply *reply);

   // Streams chirps from all followed users, returns a stream MonitorReply
   grpc::Status monitor(grpc::ServerContext *context, const chirp::MonitorRequest *request, grpc::ServerWriter<chirp::MonitorReply> *stream);

 private:
   // Part of the service layer than communicates with Backend Key Value Store Implementation over GRPC
   BackendClient backend_client;

};


