#ifndef SRC_CHIRP_BACKEND_CLIENT_H_
#define SRC_CHIRP_BACKEND_CLIENT_H_

#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>

#include "../key_value_store.grpc.pb.h"

#include "key_value_store_interface.h"

// Client for service layer.
// takes requests from service layer and dispatches to backend_server thru GRPC
class BackendClient : public KeyValueStoreInterface {
 public:
  virtual ~BackendClient(){};
  std::optional<std::string> Get(const std::string &key);
  // Constructs a payload to send to backend_server layer backend thru GRPC
  // Will be received by put() in backend_server
  // returns true if PUT was successful.
  bool Put(const std::string &key, const std::string &value);
  // Constructs a payload to send to backend_server layer backend thru GRPC
  // Will be received by del() in backend_server
  // returns true if DELKEY was successful
  bool Del(const std::string &key);

 private:
  std::shared_ptr<grpc::Channel> channel_ = grpc::CreateChannel(
      "localhost:50002", grpc::InsecureChannelCredentials());
  // stub to communicate with KeyValueStore
  std::unique_ptr<chirp::KeyValueStore::Stub> stub_ =
      chirp::KeyValueStore::NewStub(channel_);
  // pointer used to communicate with service layer
  // std::unique_ptr<chirp::KeyValueStore::Stub> stub_;
};

#endif  // CHIRP_BACKEND_CLIENT_H_
