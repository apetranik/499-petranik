#include "backend_client.h"
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <gflags/gflags.h>
#include <grpcpp/grpcpp.h>

#include "key_value_store.grpc.pb.h"

std::string BackendClient::SendGetRequest(const std::string &key) {
  grpc::ClientContext context;
  std::shared_ptr<grpc::ClientReaderWriter<chirp::GetRequest, chirp::GetReply>> stream(stub_->get(&context));

  // write request to stream
  std::thread write_to_stream([&key, &stream]() {
    chirp::GetRequest request;
    request.set_key(key);
    stream->Write(request);
    stream->WritesDone();
  });

  // Read reply from stream and write back as value
  chirp::GetReply reply;
  std::string value;
  stream->Read(&reply);
  value = reply.value();

  write_to_stream.join();
  grpc::Status status = stream->Finish();

  return value;
}

bool BackendClient::SendPutRequest(const std::string &key, const std::string &value) {
  grpc::ClientContext context;

  // construct PutRequest and send to backend
  chirp::PutRequest request;
  chirp::PutReply reply;

  request.set_key(key);
  request.set_value(value);

  grpc::Status status = stub_->put(&context, request, &reply);

  if(status.ok()) {
    return true;
  }

  return false;
}

bool BackendClient::SendDeleteKeyRequest(const std::string &key) {
  grpc::ClientContext context;

  chirp::DeleteRequest request;
  chirp::DeleteReply reply;

  request.set_key(key);

  grpc::Status status = stub_->deletekey(&context, request, &reply);

  if(status.ok()) {
    return true;
  }
  
  return false;
}
