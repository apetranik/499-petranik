#include "chirp_client.h"

DEFINE_string(stream, "", "stream all chirps with this hashtag");
DEFINE_string(register, "", "register user in key value store");
DEFINE_string(user, "", "current user of chirp");
DEFINE_string(chirp, "", "string to chirp out");
DEFINE_string(reply, "", "string to reply to a given chirp");
DEFINE_string(read, "", "Reads the chirp thread starting at the given id");
DEFINE_string(follow, "", "Starts following the given username");
DEFINE_bool(monitor, false, "Streams new tweets from those currently followed");

int ChirpClient::registeruser(const std::string &user) {

  grpc::ClientContext context;
  chirp::RegisterRequest request;
  chirp::RegisterReply reply; // Data from server will be updated here

  request.set_username(user);

  // RPC call
  grpc::Status status = stub_->registeruser(&context, request, &reply);

  std::string logging_message;
  // User already exists in key value store
  if (status.error_code() == grpc::StatusCode::INVALID_ARGUMENT) {
    logging_message = "Failure to register '" + user + "'";
    logging_message += "\n" + status.error_message();
    LOG(ERROR) << "\n" << logging_message << std::endl;
    return 1;
  }
  // Registration was successful
  if (status.ok()) {
    logging_message = "Successfully registered user: '" + user + "'";
    std::cout << logging_message << std::endl;
    LOG(INFO) << logging_message << std::endl;
    return 0;
  }
}

int ChirpClient::chirp(const std::string &user, const std::string &text,
                       const std::string &parent_id) {
  grpc::ClientContext context;
  chirp::ChirpRequest request;
  chirp::ChirpReply reply;

  request.set_username(user);
  request.set_text(text);
  request.set_parent_id(parent_id);

  // RPC call
  grpc::Status status = stub_->chirp(&context, request, &reply);

  std::string logging_message;
  // User doesn't exist or reply ID doesnt exist so chirp failed
  if (status.error_code() == grpc::StatusCode::NOT_FOUND) {
    logging_message = "Failed chirp\n" + status.error_message();
    LOG(ERROR) << "\n" << logging_message << std::endl;
    return 1;
  }
  // Chirp was successful
  if (status.ok()) {
    logging_message = "user: '" + user + "' successfully chirped '" +
                      reply.chirp().text() + "' (ID: " + reply.chirp().id() +
                      ")";

    if (!reply.chirp().parent_id().empty()) {
      logging_message += "\nReplied to ID: " + reply.chirp().parent_id();
    }
    std::cout << logging_message << std::endl;
    LOG(INFO) << logging_message << std::endl;
    return 0;
  }
}

int ChirpClient::follow(const std::string &user,
                        const std::string &user_to_follow) {
  grpc::ClientContext context;
  chirp::FollowRequest request;
  chirp::FollowReply reply; // Data from server will be updated here

  request.set_username(user);
  request.set_to_follow(user_to_follow);

  // RPC call
  grpc::Status status = stub_->follow(&context, request, &reply);

  std::string logging_message;
  // User doesn't exist so follow failed
  if (status.error_code() == grpc::StatusCode::NOT_FOUND) {
    logging_message = "Failed follow attempt\n" + status.error_message();
    LOG(ERROR) << "\n" << logging_message << std::endl;
    return 1;
  }
  // User is already following user to follow
  if (status.error_code() == grpc::StatusCode::ALREADY_EXISTS) {
    logging_message = "Failed follow\n" + status.error_message();
    LOG(ERROR) << "\n" << logging_message << std::endl;
    return 1;
  }
  // Follow was successful
  if (status.ok()) {
    logging_message =
        "user: '" + user + "' successfully followed '" + user_to_follow + "'";
    std::cout << logging_message << std::endl;
    LOG(INFO) << logging_message << std::endl;
    return 0;
  }
}

int ChirpClient::read(const std::string &chirp_id) {
  grpc::ClientContext context;
  chirp::ReadRequest request;
  chirp::ReadReply reply; // Data from server will be updated here

  request.set_chirp_id(chirp_id);

  // RPC call
  grpc::Status status = stub_->read(&context, request, &reply);

  std::string logging_message;
  // Chirp ID was not found in key value store
  if (status.error_code() == grpc::StatusCode::NOT_FOUND) {
    logging_message =
        "Failure to read ID: " + chirp_id + "\n" + status.error_message();
    LOG(ERROR) << "\n" << logging_message << std::endl;
    return 1;
  }
  // Read was successful, print out thread
  if (status.ok()) {
    std::vector<chirp::Chirp> reply_chirps;
    std::copy(reply.chirps().begin(), reply.chirps().end(),
              std::back_inserter(reply_chirps));
    PrintChirpThread(reply_chirps, true);
    return 0;
  }
  // Other failure
  else {
    logging_message =
        "Failure to read ID: " + chirp_id + "\n" + status.error_message();
    LOG(ERROR) << "\n" << logging_message << std::endl;
    return 1;
  }
}

int ChirpClient::monitor(const std::string &user) {

  grpc::ClientContext context;
  chirp::MonitorRequest request;
  chirp::MonitorReply reply; // Data from service layer will be updated here

  request.set_username(user);
  // error check and give info to user about followers
  if (!CheckMonitorInfo(user)) {
    return 1; // return if monitoring check failed and monitoring will fail
  }

  std::unique_ptr<grpc::ClientReader<chirp::MonitorReply>> reader(
      stub_->monitor(&context, request));

  std::vector<chirp::Chirp> chirps;
  std::string logging_message;
  // RPC call - read in chirps from stream continously
  while (true) {
    if (reader->Read(&reply)) {
      chirps.push_back(reply.chirp());
      PrintChirpThread(chirps, false);
      chirps.clear();
    }
  }

  grpc::Status status = reader->Finish();
  // Monitoring stopped
  if (status.ok()) {
    logging_message = "Monitoring ended\n" + status.error_message();
    std::cout << logging_message << std::endl;
    LOG(INFO) << logging_message << std::endl;
    return 0;
  }
  // Other
  else {
    logging_message = "Monitoring stopped\n" + status.error_message();
    LOG(ERROR) << "\n" << logging_message << std::endl;
    return 1;
  }
}

bool ChirpClient::CheckMonitorInfo(const std::string &user) {
  grpc::ClientContext context;
  chirp::MonitorRequest request;
  request.set_username(user);

  // Check user info for monitoring
  chirp::Followers reply;
  grpc::Status status =
      stub_->validate_monitor_request(&context, request, &reply);

  std::string logging_message;
  // User doesn't exist
  if (status.error_code() == grpc::StatusCode::NOT_FOUND) {
    logging_message = "Failed monitor for user '" + user + "'";
    logging_message += status.error_message();
    LOG(ERROR) << "\n" << logging_message << std::endl;
    return false;
  }
  // Warn user if they don't follow anyone
  if (status.error_code() == grpc::StatusCode::UNIMPLEMENTED) {
    logging_message = "Warning: user '" + user + "' " + status.error_message();
    LOG(WARNING) << "\n" << logging_message << std::endl;
    std::cout << logging_message << std::endl;
    return true;
  }
  // Tell user who they follow
  if (status.ok()) {
    logging_message = "Currently following: " + reply.names() + "\n";
    std::cout << logging_message << std::endl;
    LOG(INFO) << logging_message << std::endl;
    return true;
  }
}

void ChirpClient::PrintChirpThread(
    const std::vector<chirp::Chirp> &reply_chirps, bool isThread) {
  // only need to print one chirp
  std::string thread_string = "";
  if (!isThread) {
    thread_string += "|----------------------------------|\n";
    thread_string += "  [" + reply_chirps.at(0).username() + "]\n";
    thread_string += "  ID: " + reply_chirps.at(0).id();
    if (!reply_chirps.at(0).parent_id().empty()) {
      thread_string += ", (reply to: " + reply_chirps.at(0).parent_id() + ")";
    }
    thread_string += "\n";

    thread_string += "  - '" + reply_chirps.at(0).text() + "'\n";
    thread_string += google::protobuf::util::TimeUtil::ToString(
        google::protobuf::util::TimeUtil::SecondsToTimestamp(
            reply_chirps.at(0).timestamp().seconds()));
    thread_string += "\n|----------------------------------|\n";
  }

  // Is a thread so we need to tab correctly
  else {
    std::string tab = "\t";
    std::string total_tabs = "";
    for (const chirp::Chirp chirp : reply_chirps) {
      total_tabs = "";

      for (int i = 1; i < chirp.depth(); ++i) {
        total_tabs += tab;
      }
      thread_string +=
          "\n" + total_tabs + "|----------------------------------|\n";
      thread_string += total_tabs + "  [" + chirp.username() + "]\n";
      thread_string += total_tabs + "  ID: " + chirp.id();
      if (!chirp.parent_id().empty()) {
        thread_string += ", (reply to ID: " + chirp.parent_id() + ")";
      }
      thread_string += "\n";
      thread_string += total_tabs + "  - '" + chirp.text() + "'\n";
      thread_string += total_tabs + "  " +
                       google::protobuf::util::TimeUtil::ToString(
                           google::protobuf::util::TimeUtil::SecondsToTimestamp(
                               chirp.timestamp().seconds()));

      thread_string +=
          "\n" + total_tabs + "|----------------------------------|";
    }
  }
  std::cout << thread_string << std::endl;
  LOG(INFO) << thread_string << std::endl;
}
// Cindy's Implementation
bool ChirpClient::stream(const std::string &hashtagword) {
  grpc::ClientContext context;
  chirp::StreamRequest request;
  chirp::StreamReply reply;
  request.set_hashtag(hashtagword);

  std::unique_ptr<grpc::ClientReader<chirp::StreamReply>> reader(
      stub_->stream(&context, request));
  // TODO: output the streams here.
  // Sucessfully able to call functions through grpc for streaming hashtags
  std::vector<chirp::Chirp> chirps;
  std::string logging_message;
  // RPC call - read in chirps from stream continously
  while (true) {
    if (reader->Read(&reply)) {
    }
  }

  grpc::Status status = reader->Finish();
  // Monitoring stopped
  if (status.ok()) {
    logging_message = "Streaming ended\n" + status.error_message();
    std::cout << logging_message << std::endl;
    LOG(INFO) << logging_message << std::endl;
    return false;
  } else {
    logging_message = "Streaming stopped\n" + status.error_message();
    LOG(ERROR) << "\n" << logging_message << std::endl;
    return true;
  }
  return true;
}
// Instantiate client. It requires a channel, out of which the actual RPCs
// are created. This channel models a connection to an endpoint (in this case,
// localhost at port 50000). We indicate that the channel isn't authenticated
// (use of InsecureChannelCredentials()).
int main(int argc, char **argv) {
  ChirpClient chirp_client(grpc::CreateChannel(
      "localhost:50000", grpc::InsecureChannelCredentials()));
  google::SetLogDestination(google::GLOG_INFO, "../logs/");
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  std::string register_user = FLAGS_register;
  std::string user = FLAGS_user;
  std::string chirp = FLAGS_chirp;
  std::string parent_id = FLAGS_reply;
  std::string read_id = FLAGS_read;
  std::string follow = FLAGS_follow;
  std::string stream = FLAGS_stream;
  bool monitor = FLAGS_monitor;

  if (!register_user.empty()) {
    return chirp_client.registeruser(register_user);
  }
  if (!read_id.empty()) {
    return chirp_client.read(read_id);
  }
  if (!parent_id.empty() && chirp.empty()) {
    LOG(WARNING)
        << "User attempted to reply without specifying chirp ID to reply to"
        << std::endl;
    std::cout << "Must specific chirp ID to reply to " << std::endl;
  }
  if (!user.empty()) {
    if (!chirp.empty()) {
      return chirp_client.chirp(user, chirp, parent_id);
    }
    if (!follow.empty()) {
      return chirp_client.follow(user, follow);
    }
    if (!stream.empty() && stream[0] == '#' && stream.length() > 1) {
      return chirp_client.stream("test");
    } else if (stream.length() <= 1) {
      LOG(ERROR) << "\n hashtag must not be empty" << std::endl;
    }
    if (monitor) {
      return chirp_client.monitor(user);
    }
  }
  LOG(ERROR) << "\nInvalid command" << std::endl;
  return 1;
}
