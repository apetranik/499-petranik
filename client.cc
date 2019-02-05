#include "client.h"
#include <iostream>
#include <memory>
#include <string>

#include <gflags/gflags.h>
#include <grpcpp/grpcpp.h>

#include "service.grpc.pb.h"

DEFINE_string(user, "", "");
DEFINE_string(chirp, "", "");

chirp::Chirp Client::chirp(const std::string& user, const std::string& text, const std::string& parent_id) {
  // Set request params
  chirp::ChirpRequest request;
  request.set_username(user);
  request.set_text(text);
  request.set_parent_id(parent_id);

  // Data from server will be updated here
  chirp::ChirpReply reply;

  grpc::ClientContext context;
  // RPC call
  grpc::Status status = stub_->chirp(&context, request, &reply);

  // Reply with chirp if status is ok, else reply with empty chirp (fow now)
  if (status.ok()) {
    return reply.chirp();

  } 
  else {
    std::cout << status.error_code() << ": " << status.error_message() << std::endl;
    chirp::Chirp chirp;
    return chirp;
  }
  /* TODO: 
    - Handle reply
    - Do something more useful if reply is null
    - handle front end CLI chirp request
  */
}

// Instantiate client. It requires a channel, out of which the actual RPCs
// are created. This channel models a connection to an endpoint (in this case,
// localhost at port 50051). We indicate that the channel isn't authenticated
// (use of InsecureChannelCredentials()).
int main(int argc, char** argv) {
  Client greeter(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));

  gflags::ParseCommandLineFlags(&argc, &argv, true);
  std::string user = FLAGS_user;
  std::string chirp = FLAGS_chirp;
  std::string parent_id("1");

  chirp::Chirp reply = greeter.chirp(user, chirp, parent_id);
  std::cout << "Client received chirp " << std::endl;

  return 0;
}
