#include <service.h>

#include <iostream>
#include <memory>
#include <string>
#include <map>

#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/impl/codegen/status.h>


grpc::Status Backend::MakePutRequest(grpc::ServerContext *context, const chirp::PutRequest *request, chirp::PutReply *reply) {
  /* TODO:
  	- Create PutRequest proto
    - Read PUT: request
    - Add item to backend key,value store
    - Return status
  */

}
grpc::Status Backend::MakeGetRequest(grpc::ServerContext *server, grpc::ServerReaderWriter<chirp::GetReply, chirp::GetRequest> *stream) {
  /* TODO:
    - Create GetReply and GetRequest proto
    - Read GET request
    - Create reply and write to stream
  */ 
}

grpc::Status Backend::MakeDelRequest(grpc::ServerContext *server, const chirp::DelRequest *request, chirp::DelReply *reply) {
	/* TODO:
	- create DelRequest and DelReply protos
    - Read DEL request
    - Remove item from backend key,value store
    - Return status
  */ 
}
