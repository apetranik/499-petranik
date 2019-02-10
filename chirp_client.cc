#include "chirp_client.h"
#include <iostream>
#include <memory>
#include <string>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <grpcpp/grpcpp.h>

#include "service.grpc.pb.h"

DEFINE_string(user, "default_user", "current user of chirp");
DEFINE_string(chirp, "default_chirp", "string to chirp out");
DEFINE_string(reply, "", "string to reply to a given chirp");

bool ChirpClient::registeruser(const std::string& user) {
  // Set request params
  chirp::RegisterRequest request;
  request.set_username(user);

  // Data from server will be updated here
  chirp::RegisterReply reply;

  grpc::ClientContext context;
  // RPC call
  grpc::Status status = stub_->registeruser(&context, request, &reply);

  // Reply with true if status is ok, else reply false (failed) for now
  if (status.ok()) {
    return true;
  } 
  else {
    std::cout << status.error_code() << ": " << status.error_message() << std::endl;
    return false;
  }
  /* TODO: 
    - handle failed registration
  */
}

chirp::Chirp ChirpClient::chirp(const std::string& user, const std::string& text, const std::string& parent_id) {
  // Set request params
  chirp::ChirpRequest request;
  request.set_username(user);
  request.set_text(text);
  request.set_parent_id(parent_id);

  // Data from server will be updated here
  chirp::ChirpReply *reply = new chirp::ChirpReply();

  grpc::ClientContext context;
  // RPC call
  grpc::Status status = stub_->chirp(&context, request, reply);

  if (status.ok()) {
    return reply->chirp();
  } 
  else {
    std::cout << status.error_code() << ": " << status.error_message() << std::endl;
    chirp::Chirp chirp;
    return chirp;
  }
  /* TODO: 
    - Do something more useful if reply is null
    - handle front end CLI chirp request
  */
}

bool ChirpClient::follow(const std::string& user, const std::string& user_to_follow) {
  // Set request params
  chirp::FollowRequest request;
  request.set_username(user);
  request.set_to_follow(user_to_follow);

  // Data from server will be updated here
  chirp::FollowReply reply;

  grpc::ClientContext context;
  // RPC call
  grpc::Status status = stub_->follow(&context, request, &reply);

  // Reply with true if status is ok, else reply false (failure) for now
  if (status.ok()) {
    return true;
  } 
  else {
    std::cout << status.error_code() << ": " << status.error_message() << std::endl;
    return false;
  }
  /* TODO: 
    - handle failed follow request
  */
}

bool ChirpClient::read(const std::string& chirp_id) {
  // Set request params
  chirp::ReadRequest request;
  request.set_chirp_id(chirp_id);

  // Data from server will be updated here
  chirp::ReadReply reply;

  grpc::ClientContext context;
  // RPC call
  grpc::Status status = stub_->read(&context, request, &reply);

  // Reply with true if status is ok, else reply false (failure) for now
  if (status.ok()) {
    return true;
  } 
  else {
    std::cout << status.error_code() << ": " << status.error_message() << std::endl;
    return false;
  }
  /* TODO: 
    - handle failed read of chirps
  */
}

chirp::Chirp ChirpClient::monitor(const std::string& user) {
  // Set request params
  chirp::MonitorRequest request;
  request.set_username(user);

  // Data from service layer will be updated here
  chirp::MonitorReply reply;

  grpc::ClientContext context;

  // RPC call
  std::unique_ptr<grpc::ClientReader<chirp::MonitorReply> > reader(stub_->monitor(&context, request));

  // Read reply from stream
  while (reader->Read(&reply)) {
    LOG(INFO) << "Received chirp back" << std::endl;
    LOG(INFO) << reply.chirp().username() << std::endl;
  }

  grpc::Status status = reader->Finish();

  if (status.ok()) {
    return reply.chirp();
  } 
  else {
    std::cout << status.error_code() << ": " << status.error_message() << std::endl;
    chirp::Chirp chirp;
    return chirp;
  }
  /* TODO: 
    - handle failed read of chirps
  */
}

// Instantiate client. It requires a channel, out of which the actual RPCs
// are created. This channel models a connection to an endpoint (in this case,
// localhost at port 50000). We indicate that the channel isn't authenticated
// (use of InsecureChannelCredentials()).
int main(int argc, char** argv) {
  ChirpClient greeter(grpc::CreateChannel("localhost:50000", grpc::InsecureChannelCredentials()));
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  std::string user = FLAGS_user;
  std::string chirp = FLAGS_chirp;
  std::string parent_id = FLAGS_reply;
  if(parent_id.empty()) {
    parent_id = "0";
  }
  
  bool register_success = greeter.registeruser(user);
  chirp::Chirp response = greeter.chirp(user, chirp, parent_id);

  return 0;
}
