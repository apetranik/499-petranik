#ifndef SRC_CHIRP_SERVICE_CONTROLLER_H_
#define SRC_CHIRP_SERVICE_CONTROLLER_H_

#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <google/protobuf/repeated_field.h>
#include <google/protobuf/util/time_util.h>
#include <grpcpp/grpcpp.h>

#include "backend_client.h"
#include "service.h"

#include "../data_storage_types.grpc.pb.h"
#include "../service.grpc.pb.h"

// A backend service to receive requests from client (command line)
class ServiceController final : public chirp::ServiceLayer::Service {
 public:
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
  // while monitoring, write new chirps to stream
  void stream_new_chirps(chirp::User &user,
                         grpc::ServerWriter<chirp::MonitorReply> *stream,
                         chirp::MonitorReply &reply);
  // Part of the service layer than communicates with Backend Key Value Store
  // Either over grpc or straight to KVS (for unit testing)
  BackendClient backend_client_;
  ServiceLayer service_ = ServiceLayer(&backend_client_);
};

#endif
