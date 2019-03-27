#ifndef CHIRP_SERVICE_H_
#define CHIRP_SERVICE_H_

#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <google/protobuf/repeated_field.h>
#include <google/protobuf/util/time_util.h>
#include <grpcpp/grpcpp.h>

#include "data_storage_types.grpc.pb.h"
#include "service.grpc.pb.h"

#include "backend_client.h"

// A backend service to receive requests from client (command line)
class Service final : public chirp::ServiceLayer::Service {
 public:
  Service()
      : backend_client_(grpc::CreateChannel(
            "localhost:50002", grpc::InsecureChannelCredentials())) {}
  // Registers the given non-blank username
  grpc::Status registeruser(grpc::ServerContext *context,
                            const chirp::RegisterRequest *request,
                            chirp::RegisterReply *reply);
  // Posts a new chirp (optionally as a reply)
  grpc::Status chirp(grpc::ServerContext *context,
                     const chirp::ChirpRequest *request,
                     chirp::ChirpReply *reply);
  // Starts following a given user
  grpc::Status follow(grpc::ServerContext *context,
                      const chirp::FollowRequest *request,
                      chirp::FollowReply *reply);
  // Reads a chirp thread from the given id
  grpc::Status read(grpc::ServerContext *context,
                    const chirp::ReadRequest *request, chirp::ReadReply *reply);
  // Streams chirps from all followed users, returns a stream MonitorReply
  grpc::Status monitor(grpc::ServerContext *context,
                       const chirp::MonitorRequest *request,
                       grpc::ServerWriter<chirp::MonitorReply> *stream);
  // Checks if user exists, if user is following anyone to monitor and if so,
  // lists who the user is following
  grpc::Status validate_monitor_request(grpc::ServerContext *context,
                                        const chirp::MonitorRequest *request,
                                        chirp::Followers *reply);

 private:
  // Helper function - performs DFS on chirp thread to collect all chirps and
  // replies to chirps
  void ThreadDFS(chirp::ReadReply *reply, std::string chirp_id, int depth);
  // Registers a user as "monitoring" with everyone they follow
  void register_monitoring_user(chirp::User &user,
                                std::vector<std::string> &followed_by_user,
                                const std::string &username);
  // Clear cached chirps & remove user as monitoring from users theyre following
  void terminate_monitor(chirp::User &user,
                         std::vector<std::string> &followed_by_user,
                         const std::string &username);
  // while monitoring, write new chirps to stream
  void stream_new_chirps(chirp::User &user,
                         grpc::ServerWriter<chirp::MonitorReply> *stream,
                         chirp::MonitorReply &reply);
  // Part of the service layer than communicates with Backend Key Value Store
  // Implementation over GRPC
  BackendClient backend_client_;
  // mutex used for concurrency
  std::mutex mutex_;
};

#endif
