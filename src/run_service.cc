
#include <grpcpp/grpcpp.h>
#include "service_controller.h"

// Sets up GRPC and starts server
void run_service() {
  const std::string server_address("0.0.0.0:50000");
  ServiceController service;
  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server is listening on " << server_address << std::endl;
  server->Wait();
}

int main(int argc, char **argv) {
  run_service();
  return 0;
}
