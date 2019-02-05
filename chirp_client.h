#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>

#include "service.grpc.pb.h"
// Handles command line input and sends requests to service layer thru GRPC
class ChirpClient {
 public:
   // constructs a Client object that accepts input from CLI and dispatches to service layer
   ChirpClient(std::shared_ptr<grpc::Channel> channel) : stub_(chirp::ServiceLayer::NewStub(channel)) {}
   // Constructs a payload to send to service layer backend thru grpc
   chirp::Chirp chirp(const std::string& user, const std::string& text, const std::string& parent_id);  
 private:
  // pointer used to communicate with service layer
  std::unique_ptr<chirp::ServiceLayer::Stub> stub_;
};

