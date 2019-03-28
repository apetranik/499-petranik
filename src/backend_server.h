#ifndef SRC_CHIRP_BACKEND_SERVER_H_
#define SRC_CHIRP_BACKEND_SERVER_H_

#include <iostream>
#include <optional>
#include <string>
#include <utility>

#include <grpcpp/grpcpp.h>

#include "../key_value_store.grpc.pb.h"
#include "key_value_store.h"

// Implementation of key value store
// Serves as backend to receive requests from service layer
// Implements get, put and deletekey.
// inherits from the `chirp::KeyValueStore::Service`
class KeyValueStoreImpl final : public chirp::KeyValueStore::Service {
 public:
  // Accepts put requests from service layer and modifies key_value_store_
  grpc::Status put(grpc::ServerContext *context,
                   const chirp::PutRequest *request, chirp::PutReply *reply);
  // Accepts get requests from service layer and modifies key_value_store_
  grpc::Status get(
      grpc::ServerContext *context,
      grpc::ServerReaderWriter<chirp::GetReply, chirp::GetRequest> *stream);
  // Accepts deletekey requests from service layer and modifies key_value_store_
  grpc::Status deletekey(grpc::ServerContext *context,
                         const chirp::DeleteRequest *request,
                         chirp::DeleteReply *reply);

 private:
  // Actual backend data structure
  KeyValueStore key_value_store_;
};

#endif  // CHIRP_BACKEND_SERVER_H_
