#include "backend_server.h"

#include <map>
#include <string>
#include <utility>

#include <grpcpp/grpcpp.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>

#include "key_value_store.h"
#include "key_value_store.grpc.pb.h"

grpc::Status KeyValueStoreImpl::put(grpc::ServerContext *context, const chirp::PutRequest *request, chirp::PutReply *reply) {
  if (request == nullptr) {
    return grpc::Status(grpc::INVALID_ARGUMENT,"`PutRequest` is a nullptr","");
  }

  bool success = key_value_store_.Put(request->key(), request->value());

  if (success) {
    return grpc::Status::OK;
  }
  else {
    return grpc::Status(grpc::UNKNOWN, "Unknown error", "");
  }
}

grpc::Status KeyValueStoreImpl::get(grpc::ServerContext *context, grpc::ServerReaderWriter<chirp::GetReply, chirp::GetRequest> *stream) {
  chirp::GetRequest request;

  while(stream->Read(&request)) {
    chirp::GetReply reply;
    std::string value;

    // gets value from optional or if returned {} save an empty reply
    auto optional_reply = key_value_store_.Get(request.key()).value_or("");
    reply.set_value(value);

    reply.set_value("");
    stream->Write(reply);
  }

  return grpc::Status::OK;
}

grpc::Status KeyValueStoreImpl::deletekey(grpc::ServerContext *context,const chirp::DeleteRequest *request, chirp::DeleteReply *reply) {
  if (request == nullptr) {
    return grpc::Status(grpc::INVALID_ARGUMENT, "`DeleteRequest` is a nullptr", "");
  }

  bool success = key_value_store_.Del(request->key());

  if (success) {
    return grpc::Status::OK;
  }
  else {
    return grpc::Status(grpc::NOT_FOUND, "Unknown error", "");
  }
}

void run_server() {
  std::string server_address("0.0.0.0:50002");
  KeyValueStoreImpl service;

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Backend Server is listening on " << server_address << std::endl;
  server->Wait();
}

int main(int argc, char **argv) {
  run_server();

  return 0;
}
