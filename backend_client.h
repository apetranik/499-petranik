#include <iostream>
#include <memory>
#include <string>
#include <optional>

#include <grpcpp/channel.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>

#include "key_value_store.grpc.pb.h"

// Client for service layer.
// takes requests from service layer and dispatches to backend_server thru GRPC
class BackendClient {
 public:
   // constructs a Backend Client object that takes in grpc info
   BackendClient(std::shared_ptr<grpc::Channel> channel) : stub_(chirp::KeyValueStore::NewStub(channel)) {}
   // Constructs a payload to send to backend_server layer backend thru GRPC
   // Will be received by get() in backend_server and return a vector of string as a reply
   std::string SendGetRequest(const std::string &key);
   // Constructs a payload to send to backend_server layer backend thru GRPC
   // Will be received by put() in backend_server
   // returns true if PUT was successful.
   bool SendPutRequest(const std::string &key, const std::string &value);
   // Constructs a payload to send to backend_server layer backend thru GRPC
   // Will be received by del() in backend_server
   // returns true if DELKEY was successful
   bool SendDeleteKeyRequest(const std::string &key);

 private:
  // pointer used to communicate with service layer
  std::unique_ptr<chirp::KeyValueStore::Stub> stub_;
};