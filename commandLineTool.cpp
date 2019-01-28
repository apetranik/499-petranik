#include "commandLineTool.h"

#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>

// Command Line client that handles interactions/inputs from the client
int main(int argc, char **argv) {
    /* TODO:
    - create service object on port 50002.
    - listen for user input
    - dispatch requests to appropriate files
    */
    CommandLineClient client(grpc::CreateChannel("localhost:50002", grpc::InsecureChannelCredentials()));
    return 0;
}
