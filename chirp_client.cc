#include "chirp_client.h"
#include <iostream>
#include <memory>
#include <string>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <grpcpp/grpcpp.h>
#include <google/protobuf/util/time_util.h>

#include "service.grpc.pb.h"

DEFINE_string(user, "", "current user of chirp");
DEFINE_string(chirp, "", "string to chirp out");
DEFINE_string(reply, "", "string to reply to a given chirp");
DEFINE_string(read, "", "Reads the chirp thread starting at the given id");


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

  grpc::ClientContext context;

  // Data from server will be updated here
  chirp::ChirpReply *reply = new chirp::ChirpReply();

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

  grpc::ClientContext context;

  // Data from server will be updated here
  chirp::ReadReply reply;

  // RPC call
  grpc::Status status = stub_->read(&context, request, &reply);

  // copy repeated chirps field into vector (for display)
  std::vector<chirp::Chirp> reply_chirps;
  std::copy(reply.chirps().begin(), reply.chirps().end(), std::back_inserter(reply_chirps));

  printChirpThread(reply_chirps); // print out thread to client

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

void ChirpClient::printChirpThread(std::vector<chirp::Chirp> &reply_chirps) {
  std::cout << "[" << reply_chirps.at(0).username() << "]" << std::endl;
  std::cout << "- " << reply_chirps.at(0).text() << std::endl;
  std::cout << google::protobuf::util::TimeUtil::ToString(google::protobuf::util::TimeUtil::SecondsToTimestamp(reply_chirps.at(0).timestamp().seconds())) << std::endl;
  std::cout << "----------------------------------" << std::endl;
  reply_chirps.erase(reply_chirps.begin());

  for(chirp::Chirp chirp : reply_chirps) {
     std::cout << "\t" << "[" << chirp.username() << "]" << std::endl;
     std::cout << "\t" << "- " << chirp.text() << std::endl;
     std::cout << "\t" << google::protobuf::util::TimeUtil::ToString(google::protobuf::util::TimeUtil::SecondsToTimestamp(chirp.timestamp().seconds())) << std::endl;
     std::cout << "\n";
  }
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
  std::string read_id = FLAGS_read;

  /*if(!user.empty() && !chirp.empty()) {
    bool register_success = greeter.registeruser(user);
  }*/

  
  if(!read_id.empty())
  {
    bool read = greeter.read(read_id);
  }
  if(!chirp.empty())
  {
    chirp::Chirp response = greeter.chirp(user, chirp, parent_id);
  }
  

  return 0;
}
