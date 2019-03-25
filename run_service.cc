#include "service.h"

#include <grpcpp/grpcpp.h>

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
