#include "backend_server.h"

grpc::Status KeyValueStoreImpl::put(grpc::ServerContext *context,
                                    const chirp::PutRequest *request,
                                    chirp::PutReply *reply) {
  if (request == nullptr) {
    return grpc::Status(grpc::INVALID_ARGUMENT, "`PutRequest` is a nullptr",
                        "");
  }

  bool success = key_value_store_.Put(request->key(), request->value());

  if (success) {
    return grpc::Status::OK;
  } else {
    return grpc::Status(grpc::UNKNOWN, "Unknown error", "");
  }
}

grpc::Status KeyValueStoreImpl::get(
    grpc::ServerContext *context,
    grpc::ServerReaderWriter<chirp::GetReply, chirp::GetRequest> *stream) {
  chirp::GetRequest request;

  while (stream->Read(&request)) {
    chirp::GetReply reply;

    // gets value from optional or if returned {} save an empty reply
    auto optional_reply = key_value_store_.Get(request.key()).value_or("");
    reply.set_value(optional_reply);

    stream->Write(reply);
  }
  return grpc::Status::OK;
}

grpc::Status KeyValueStoreImpl::deletekey(grpc::ServerContext *context,
                                          const chirp::DeleteRequest *request,
                                          chirp::DeleteReply *reply) {
  if (request == nullptr) {
    return grpc::Status(grpc::INVALID_ARGUMENT, "`DeleteRequest` is a nullptr",
                        "");
  }

  bool success = key_value_store_.Del(request->key());

  if (success) {
    return grpc::Status::OK;
  } else {
    return grpc::Status(grpc::NOT_FOUND, "Unknown error", "");
  }
}
