#include <grpcpp/grpcpp.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include "backend_server.h"

void run_server() {
  std::string server_address("0.0.0.0:50002");
  KeyValueStoreImpl service;

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Backend Server is listening on " << server_address << std::endl;
  server->Wait();
}

int main(int argc, char **argv) {
  run_server();

  return 0;
}
