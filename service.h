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
  // Check user exists to monitor before entering monitor
  grpc::Status monitor_check(grpc::ServerContext *context,
                             const chirp::MonitorRequest *request,
                             chirp::Followers *reply);

 private:
  // Helper function - performs DFS on chirp thread to collect all chirps and
  // replies to chirps
  void ThreadDFS(chirp::ReadReply *reply, std::string chirp_id, int depth);
  // Part of the service layer than communicates with Backend Key Value Store
  // Implementation over GRPC
  BackendClient backend_client_;
  // mutex used for concurrency
  std::mutex mutex_;
};

#endif
