#include <iostream>
#include <memory>
#include <string>
#include <map>

#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/impl/codegen/status.h>


class Backend {
public:
 grpc::Status put(grpc::ServerContext *context, const chirp::PutRequest *request, chirp::PutReply *reply) message get(const std::string &key);
 grpc::Status get(grpc::ServerContext *server, grpc::ServerReaderWriter<chirp::GetReply, chirp::GetRequest> *stream);
 grpc::Status del(grpc::ServerContext *server, const chirp::DeleteRequest *request, chirp::DeleteReply *reply)
};
