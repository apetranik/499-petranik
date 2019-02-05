#include "service.h"

#include <thread>

#include <grpcpp/grpcpp.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>

#include "service.grpc.pb.h"

grpc::Status Service::registeruser(grpc::ServerContext *context, const chirp::RegisterRequest *request, chirp::RegisterReply *reply) {
  return grpc::Status::OK;
  /* TODO:
  - create register user request to send to backend_key_value store thru grpc
  */
}

grpc::Status Service::chirp(grpc::ServerContext *context, const chirp::ChirpRequest *request, chirp::ChirpReply *reply) {
  std::cout << "Received request: " << std::endl;
  std::cout << "username: " << request->username() << std::endl;
  std::cout << "text: " << request->text() << std::endl;
  std::cout << "parent_id: " << request->parent_id() << std::endl;

  return grpc::Status::OK;
  /* TODO:
    - create put request to send chirp to backend_key_value store thru grpc
  */
}

grpc::Status Service::follow(grpc::ServerContext *context, const chirp::FollowRequest *request, chirp::FollowReply *reply) {
  return grpc::Status::OK;
  /* TODO:
    - create put request to register user as a follower in backend_key_value store thru grpc
  */
}

grpc::Status Service::read(grpc::ServerContext *context, const chirp::ReadRequest *request, chirp::ReadReply *reply) {
  return grpc::Status::OK;
  /* TODO:
    - create get request to read chirp and all threads from backend_key_value store thru grpc
  */
}

grpc::Status Service::monitor(grpc::ServerContext *context, const chirp::MonitorRequest *request, chirp::MonitorReply *reply) {
  return grpc::Status::OK;
  /* TODO:
    - create get request to read chirps from backend_key_value store thru grpc
    - figure out concurrency
  */
}

void run_server() {
  std::string server_address("0.0.0.0:50051");
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
