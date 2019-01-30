#include <iostream>
#include <memory>
#include <string>
#include <map>

#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/impl/codegen/status.h>

// The key-value store for Chirp. 
// Handles GET, PUT and DELETE from service layer to retrieve/store info 
class Backend {
 public:
   grpc::Status MakePutRequest(grpc::ServerContext *context, const chirp::PutRequest *request, chirp::PutReply *reply) message get(const std::string &key);
   grpc::Status MakeGetRequest(grpc::ServerContext *server, grpc::ServerReaderWriter<chirp::GetReply, chirp::GetRequest> *stream);
   grpc::Status MakeDelRequest(grpc::ServerContext *server, const chirp::DeleteRequest *request, chirp::DeleteReply *reply)
};
