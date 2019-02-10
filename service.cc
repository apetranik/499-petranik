#include "service.h"

#include <thread>

#include <google/protobuf/util/time_util.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>

#include "service.grpc.pb.h"
#include "data_storage_types.grpc.pb.h"

grpc::Status Service::registeruser(grpc::ServerContext *context, const chirp::RegisterRequest *request, chirp::RegisterReply *reply) {
  // serialize user and send to backend
  std::string value;
  chirp::User user;
  user.set_username(request->username());
  user.SerializeToString(&value); 
  backend_client.SendPutRequest(request->username(), value);

  return grpc::Status::OK;
  /* TODO:
  - return more useful status
  */
}

grpc::Status Service::chirp(grpc::ServerContext *context, const chirp::ChirpRequest *request, chirp::ChirpReply *reply) {
  // Get User from backend, update user with new chirp, send to backend to update
  chirp::User user;
  std::string current_user = backend_client.SendGetRequest(request->username());
  user.ParseFromString(current_user);

  // construct new Chirp and ChirpReply and set params based on request or fill with default values
  chirp::Chirp *user_chirp = user.add_chirps();
  user_chirp->set_username(request->username());
  user_chirp->set_text(request->text());
  user_chirp->set_parent_id(request->parent_id());

  int64_t seconds = google::protobuf::util::TimeUtil::TimestampToSeconds(google::protobuf::util::TimeUtil::GetCurrentTime());
  int64_t useconds = google::protobuf::util::TimeUtil::TimestampToMicroseconds(google::protobuf::util::TimeUtil::GetCurrentTime());

  chirp::Timestamp *stamp = new chirp::Timestamp();
  stamp->set_seconds(seconds);
  stamp->set_useconds(useconds);

  user_chirp->set_allocated_timestamp(stamp);

  user.set_username(request->username());
  user.SerializeToString(&current_user); 
  reply->set_allocated_chirp(user_chirp);


  // Get parent thread
  chirp::ChirpThread thread;
  std::string current_thread;

  current_thread = backend_client.SendGetRequest(request->parent_id());
  thread.ParseFromString(current_thread);

  // construct new Chirp and ChirpReply and set params based on request or fill with default values
  chirp::Chirp *chirp = thread.add_replies();
  chirp->set_username(request->username());
  chirp->set_text(request->text());
  chirp->set_parent_id(request->parent_id());

  chirp::Timestamp *stamp2 = new chirp::Timestamp();
  stamp2->set_seconds(seconds);
  stamp2->set_useconds(useconds);

  chirp->set_allocated_timestamp(stamp2);

  thread.SerializeToString(&current_thread); 

  // Make a put request to the backend 
  backend_client.SendPutRequest(request->username(), current_user); // associate chirp to user
  backend_client.SendPutRequest(chirp->parent_id(), current_thread); // add chirp itself

  return grpc::Status::OK;
  /* TODO:
    - if no parent thread, create new
    - handle insuccessful chirp
  */
}

grpc::Status Service::follow(grpc::ServerContext *context, const chirp::FollowRequest *request, chirp::FollowReply *reply) {
  std::cout << "Received request: " << std::endl;
  std::cout << "username: " << request->username() << std::endl;
  std::cout << "user_to_follow: " << request->to_follow() << std::endl;

  // Get User from backend, update user with new person to follow, send to backend to update
  chirp::User user;
  std::string current_data = backend_client.SendGetRequest(request->username());
  user.ParseFromString(current_data);

  // construct new Chirp and ChirpReply and set params based on request or fill with default values
  user.add_follows(request->to_follow());

  user.SerializeToString(&current_data); 
  backend_client.SendPutRequest(request->username(), current_data);

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
