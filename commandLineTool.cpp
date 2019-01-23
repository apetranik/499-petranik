#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "service.h"
class CommandLineClient
{
};
// Create command line client and create user.
int main(int argc, char **argv)
{
    // create service object on port 50002.
    CommandLineClient client(grpc::CreateChannel("localhost:50002", grpc::InsecureChannelCredentials()));

    // register user
    std::string user("user1");
    std::string reply = service.register(user);
    std::cout << "Client received: " << reply << std::endl;

    return 0;
}
