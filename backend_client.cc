#include "backend_client.h"
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <gflags/gflags.h>
#include <grpcpp/grpcpp.h>

#include "key_value_store.grpc.pb.h"

std::vector<std::string> BackendClient::SendGetRequest(const std::vector<std::string> &keys) {
  grpc::ClientContext context;

  std::shared_ptr<grpc::ClientReaderWriter<chirp::GetRequest, chirp::GetReply>> stream(stub_->get(&context));

  std::thread write_to_stream([&keys, &stream]() {
    for (const std::string &key : keys) {
      chirp::GetRequest request;
      request.set_key(key);
      stream->Write(request);
    }
    stream->WritesDone();
  });

  chirp::GetReply reply;
  std::vector<std::string> replies;

  while (stream->Read(&reply)) {
    replies.push_back(reply.value());
  }

  write_to_stream.join();
  grpc::Status status = stream->Finish();
  /* TODO: 
    - handle failed get requests and successful, but empty replies
    - figure out concurrency
  */
  return replies; // currently still returns if empty
}

bool BackendClient::SendPutRequest(const std::string &key, const std::string &value) {
  grpc::ClientContext context;

  chirp::PutRequest request;
  chirp::PutReply reply;

  request.set_key(key);
  request.set_value(value);
  std::cout << "we out here in SendPutRequest()" << std::endl;
  grpc::Status status = stub_->put(&context, request, &reply);

  if(status.ok()) {
    std::cout << "status was OK" << std::endl;
    return true;
  }
  /* TODO: 
    - handle failed put requests with more meaningful status
    - figure out concurrency
  */
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
  /* TODO: 
    - handle failed put requests with more meaningful status
    - figure out concurrency
  */
  return false;
}
