#include "chirp_client.h"
#include <iostream>
#include <memory>
#include <string>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <grpcpp/grpcpp.h>
#include <google/protobuf/util/time_util.h>

#include "service.grpc.pb.h"

DEFINE_string(register, "", "register user in key value store");
DEFINE_string(user, "", "current user of chirp");
DEFINE_string(chirp, "", "string to chirp out");
DEFINE_string(reply, "", "string to reply to a given chirp");
DEFINE_string(read, "", "Reads the chirp thread starting at the given id");
DEFINE_string(follow, "", "Starts following the given username");
DEFINE_bool(monitor, false, "Streams new tweets from those currently followed");

void ChirpClient::registeruser(const std::string& user) {
  grpc::ClientContext context;
  chirp::RegisterRequest request;
  chirp::RegisterReply reply;   // Data from server will be updated here

  request.set_username(user);
  
  // RPC call
  grpc::Status status = stub_->registeruser(&context, request, &reply);

  // User already exists in key value store
  if(status.error_code() == grpc::StatusCode::ALREADY_EXISTS) {
    std::cout << "Failure to register " << "'" << user << "'" << std::endl;
    std::cout << status.error_message() << std::endl;
    return;
  }
  // Registration was successful
  if (status.ok()) {
    std::cout << "'" << user << "' successfully registered" << std::endl;
    return;
  } 
  else {
    std::cout << "Failure to register " << "'" << user << "'" << std::endl;
    std::cout << status.error_message() << std::endl;    
    return;
  }
}

void ChirpClient::chirp(const std::string& user, const std::string& text, const std::string& parent_id) {
  grpc::ClientContext context;
  chirp::ChirpRequest request;
  chirp::ChirpReply *reply = new chirp::ChirpReply(); // Data from server will be updated here

  request.set_username(user);
  request.set_text(text);
  request.set_parent_id(parent_id);

  // RPC call
  grpc::Status status = stub_->chirp(&context, request, reply);

  // User doesn't exist or reply ID doesnt exist so chirp failed 
  if(status.error_code() == grpc::StatusCode::NOT_FOUND) {
    std::cout << "Failed chirp" << std::endl;
    std::cout << status.error_message() << std::endl;
    return;
  }
  // Chirp was successful
  if (status.ok()) {
    std::cout << "user: '" << user << "' successfully chirped '" << reply->chirp().text() << "' (ID: " << reply->chirp().id() << ")" << std::endl;
    if(!reply->chirp().parent_id().empty()) {
      std::cout << "Replied to ID: " << reply->chirp().parent_id() << std::endl;
    }
    return;
  } 
  // Other failure
  else {
    std::cout << "Failed chirp" << std::endl;
    std::cout << status.error_message() << std::endl;    
    return;
  }
}

void ChirpClient::follow(const std::string& user, const std::string& user_to_follow) {
  grpc::ClientContext context;
  chirp::FollowRequest request;
  chirp::FollowReply reply; // Data from server will be updated here
  
  request.set_username(user);
  request.set_to_follow(user_to_follow);

  // RPC call
  grpc::Status status = stub_->follow(&context, request, &reply);

  // User doesn't exist so follow failed
  if(status.error_code() == grpc::StatusCode::NOT_FOUND) {
    std::cout << "Failed follow" << std::endl;
    std::cout << status.error_message() << std::endl;
    return;
  }
  // User is already following user to follow
  if(status.error_code() == grpc::StatusCode::NOT_FOUND) {
    std::cout << "Failed follow" << std::endl;
    std::cout << status.error_message() << std::endl;
    return;
  }
  // Follow was successful
  if (status.ok()) {
    std::cout << "user: '" << user << "' successfully followed '" << user_to_follow << "'" << std::endl;
    return;
  } 
  // other error
  else {
    std::cout << "Failed follow" << std::endl;
    std::cout << status.error_message() << std::endl;    
    return;
  }
}

void ChirpClient::read(const std::string& chirp_id) {
  grpc::ClientContext context;
  chirp::ReadRequest request;
  chirp::ReadReply reply; // Data from server will be updated here

  request.set_chirp_id(chirp_id);

  // RPC call
  grpc::Status status = stub_->read(&context, request, &reply);

  // Chirp ID was not found in key value store
  if(status.error_code() == grpc::StatusCode::NOT_FOUND) {
    std::cout << "Failure to read ID: " << "'" << chirp_id << "'" << std::endl;
    std::cout << status.error_message() << std::endl;
    return;
  }
  // Read was successful, print out thread
  if (status.ok()) {
    std::vector<chirp::Chirp> reply_chirps;
    std::copy(reply.chirps().begin(), reply.chirps().end(), std::back_inserter(reply_chirps));
    PrintChirpThread(reply_chirps, true);
    return;
  }
  // Other failure
  else {
    std::cout << "Failure to read ID: " << "'" << chirp_id << "'" << std::endl;
    std::cout << status.error_message() << std::endl;
    return;
  }
}

void ChirpClient::monitor(const std::string& user) {
  grpc::ClientContext context;
  chirp::MonitorRequest request;
  chirp::MonitorReply reply; // Data from service layer will be updated here

  request.set_username(user);

  // error check and give info to user about followers
  if(!CheckMonitorInfo(user)) {
    return; // return if monitoring check failed and monitoring will fail
  } 
  
  std::unique_ptr<grpc::ClientReader<chirp::MonitorReply> > reader(stub_->monitor(&context, request));

  std::vector<chirp::Chirp> chirps;
  // RPC call - read in chirps from stream continously
  while(true) {
    if(reader->Read(&reply)) {
      chirps.push_back(reply.chirp());
      PrintChirpThread(chirps, false);
      chirps.clear();
    }
  }
  
  grpc::Status status = reader->Finish();

  // Monitoring stopped
  if(status.ok()) {
    std::cout << "Monitoring ended" << std::endl;
    std::cout << status.error_message() << std::endl;
    return;
  }
  // Other 
  else {
    std::cout << "Monitoring stopped" << std::endl;
    std::cout << status.error_message() << std::endl;
    return;
  }
}

bool ChirpClient::CheckMonitorInfo(const std::string &user) {
  grpc::ClientContext context;
  chirp::MonitorRequest request;
  request.set_username(user);

  // Check user info for monitoring
  chirp::Followers reply;
  grpc::Status status = stub_->monitor_check(&context, request, &reply);
  
  // User doesn't exist
  if(status.error_code() == grpc::StatusCode::NOT_FOUND) {
    std::cout << "Failed monitor for user '" << user << "'" << std::endl;
    std::cout << status.error_message() << std::endl;
    return false;
  }
  // Warn user if they don't follow anyone
  if(status.error_code() == grpc::StatusCode::UNIMPLEMENTED) {
    std::cout << "Warning: user '" << user << "' " << status.error_message() << std::endl;
    return true;
  }
  // Tell user who they follow
  if(status.ok()) {
    std::cout << "Currently following: " << reply.names() << std::endl;
    return true;
  }
}

void ChirpClient::PrintChirpThread(std::vector<chirp::Chirp> &reply_chirps, bool isThread) {
  // only need to print one chirp
  if(!isThread) {
    std::cout << "|----------------------------------|" << std::endl;
    std::cout << "  [" << reply_chirps.at(0).username() << "]" << std::endl;
    std::cout << "  ID: " << reply_chirps.at(0).id(); 
    if(!reply_chirps.at(0).parent_id().empty()) {
        std::cout << ", (reply to: " << reply_chirps.at(0).parent_id() << ")";
    }
    std::cout << std::endl;
    std::cout << "  - '" << reply_chirps.at(0).text()  << "'" << std::endl;
    std::cout << "  " << google::protobuf::util::TimeUtil::ToString(google::protobuf::util::TimeUtil::SecondsToTimestamp(reply_chirps.at(0).timestamp().seconds())) << std::endl;
    std::cout << "|----------------------------------|" << std::endl;
  }

  // Is a thread so we need to tab correctly
  else {
    std::string tab = "\t";
    std::string total_tabs = "";
    for(chirp::Chirp chirp : reply_chirps) {
      total_tabs = "";

      for(int i = 1; i < chirp.depth(); ++i) {
        total_tabs += tab;
      }
      std::cout << total_tabs << "|----------------------------------|" << std::endl;
      std::cout << total_tabs << "  [" << chirp.username() << "]" << std::endl;
      std::cout << total_tabs <<"  ID: " << chirp.id();
      if(!chirp.parent_id().empty()) {
        std::cout << ", (reply to ID: " << chirp.parent_id() << ")";
      }
      std::cout << std::endl;
      std::cout << total_tabs << "  - '" << chirp.text() << "'" << std::endl;
      std::cout << total_tabs << "  " << google::protobuf::util::TimeUtil::ToString(google::protobuf::util::TimeUtil::SecondsToTimestamp(chirp.timestamp().seconds())) << std::endl;
      std::cout << total_tabs << "|----------------------------------|" << std::endl;
    }
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
  std::string register_user = FLAGS_register;
  std::string user = FLAGS_user;
  std::string chirp = FLAGS_chirp;
  std::string parent_id = FLAGS_reply;
  std::string read_id = FLAGS_read;
  std::string follow = FLAGS_follow;
  bool monitor = FLAGS_monitor;

  if(!register_user.empty()) {
    greeter.registeruser(register_user);
    return 0;
  }
  if(!read_id.empty())
  {
    greeter.read(read_id);
    return 0;
  }
  if(!parent_id.empty() && chirp.empty()) {
    std::cout << "Must specific chirp ID to reply to " << std::endl;
  }
  if(!chirp.empty() && !user.empty())
  {
    greeter.chirp(user, chirp, parent_id);
    return 0;
  }
  if(!follow.empty() && !user.empty()) {
     greeter.follow(user, follow);
     return 0;
  }
  if(monitor && !user.empty())
  {
    greeter.monitor(user);
    return 0;
  }
  else {
    std::cout << "Invalid command" << std::endl;
  }
  return 0;
}
