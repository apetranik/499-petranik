#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include <service.h>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

// Outline for Service layer
// Not implemented yet
message Service::chirp(const std::string &user, const std::string &text, int chirpID)
{
    return message;
}
// Registers the given non-blank username
message Service::registerUser(const std::string &user)
{
    return message;
}
// Posts a new chirp (optionally as a reply)
message Service::follow(const std::string &user, const std::string &userToFollow)
{
    return message;
}
// Starts following a given user
message Service::read(int chirpID)
{
    return message;
}
//Reads a chirp thread from the given id
message Service::monitor(const std::string &user)
{
    return message;
}
// Streams chirps from all followed users
message Service::monitor(const std::string &user)
{
    return message;
}
