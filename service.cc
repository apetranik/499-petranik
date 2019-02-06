#include "service.h"

#include <thread>

#include <google/protobuf/util/time_util.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>

#include "service.grpc.pb.h"

grpc::Status Service::registeruser(grpc::ServerContext *context, const chirp::RegisterRequest *request, chirp::RegisterReply *reply) {
  std::cout << "Received request: " << std::endl;
  std::cout << "username: " << request->username() << std::endl;

  return grpc::Status::OK;
  /* TODO:
  - create register user request to send to backend_key_value store thru grpc
  */
}

grpc::Status Service::chirp(grpc::ServerContext *context, const chirp::ChirpRequest *request, chirp::ChirpReply *reply) {
  std::cout << "Received request: " << std::endl;
  std::cout << "username: " << request->username() << std::endl;
  std::cout << "text: " << request->text() << std::endl;
  std::cout << "parent_id: " << request->parent_id() << std::endl;

  // construct new Chirp and ChirpReply and set params based on request or fill with default values
  chirp::Chirp *chirp = new chirp::Chirp();
  chirp->set_username(request->username());
  chirp->set_text(request->text());
  chirp->set_parent_id(request->parent_id());

  int64_t seconds = google::protobuf::util::TimeUtil::TimestampToSeconds(google::protobuf::util::TimeUtil::GetCurrentTime());
  int64_t useconds = google::protobuf::util::TimeUtil::TimestampToMicroseconds(google::protobuf::util::TimeUtil::GetCurrentTime());

  chirp::Timestamp *stamp = new chirp::Timestamp();
  stamp->set_seconds(seconds);
  stamp->set_useconds(useconds);

  chirp->set_allocated_timestamp(stamp);
  reply->set_allocated_chirp(chirp);

  return grpc::Status::OK;
  /* TODO:
    - create put request to send chirp to backend_key_value store thru grpc
  */
}

grpc::Status Service::follow(grpc::ServerContext *context, const chirp::FollowRequest *request, chirp::FollowReply *reply) {
  std::cout << "Received request: " << std::endl;
  std::cout << "username: " << request->username() << std::endl;
  std::cout << "user_to_follow: " << request->to_follow() << std::endl;
  return grpc::Status::OK;
  /* TODO:
    - create put request to register user as a follower in backend_key_value store thru grpc
  */
}

grpc::Status Service::read(grpc::ServerContext *context, const chirp::ReadRequest *request, chirp::ReadReply *reply) {
  std::cout << "Received request: " << std::endl;
  std::cout << "chirp_id: " << request->chirp_id() << std::endl;
  return grpc::Status::OK;
  /* TODO:
    - create get request to read chirp and all threads from backend_key_value store thru grpc
  */
}

  grpc::Status Service::monitor(grpc::ServerContext *context, const chirp::MonitorRequest *request, grpc::ServerWriter<chirp::MonitorReply> *stream) {
  std::cout << "Received request: monitor " << std::endl;
  std::cout << "username: " << request->username() << std::endl;

  // construct new Chirp and MonitoryReply and set params based on request or fill with default values
  chirp::MonitorReply reply;
  chirp::Chirp *chirp = new chirp::Chirp();
  chirp->set_username(request->username());
  chirp->set_text("testing_text");
  chirp->set_parent_id("testing_id_1");

  int64_t seconds = google::protobuf::util::TimeUtil::TimestampToSeconds(google::protobuf::util::TimeUtil::GetCurrentTime());
  int64_t useconds = google::protobuf::util::TimeUtil::TimestampToMicroseconds(google::protobuf::util::TimeUtil::GetCurrentTime());

  chirp::Timestamp *stamp = new chirp::Timestamp();
  stamp->set_seconds(seconds);
  stamp->set_useconds(useconds);

  chirp->set_allocated_timestamp(stamp);
  reply.set_allocated_chirp(chirp);
  stream->Write(reply); // write reply to stream based on proto

  return grpc::Status::OK;
  /* TODO:
    - create get request to read chirps from backend_key_value store thru grpc
    - figure out concurrency
  */
}

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
