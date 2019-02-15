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
   // Constructs a RegisterRequest and sends to service layer thru grpc and receives a RegisterReply back
   void registeruser(const std::string& user);
   // Constructs a FollowRequest and sends to service layer thru grpc and receives a FollowReply back
   void follow(const std::string& user, const std::string& user_to_follow);
   // Constructs a ReadRequest and sends to service layer thru grpc and receives a ReadReply back
   void read(const std::string& chirp_id);
   // Constructs a MonitorRequest and sends to service layer thru grpc and receives a stream MonitorReply back
   void monitor(const std::string& user);
   // Constructs a ChirpRequest and sends to service layer thru grpc and receives a ChirpReply back
   void chirp(const std::string& user, const std::string& text, const std::string& parent_id); 
   // Helper function - prints out chirp thread in a nice format to console 
   void PrintChirpThread(std::vector<chirp::Chirp> &reply_chirps, bool isThread);
   // Helper function - Error chekcing + info for monitor eg( missing user, following no one, who they follow)
   bool CheckMonitorInfo(const std::string &user);
 private:
  // pointer used to communicate with service layer
  std::unique_ptr<chirp::ServiceLayer::Stub> stub_;
};

