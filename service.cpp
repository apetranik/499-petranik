#include <service.h>

#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>

grpc::Status Service::chirp(const std::string &user, const std::string &text, bytes chirp_id) {
  /* TODO:
    - Create a chirp
    - Send it to backend to be stored
    - send status back
  */
}

grpc::Status Service::registerUser(const std::string &user) {
  /* TODO:
    - Create a new user
    - Save it to backend to be stored
    - send status back
  */
}

grpc::Status Service::follow(const std::string &user, const std::string &user_to_follow) {
  /* TODO:
    - Update user with a new follower in backend
    - Update user-to-follow w/ user that is now following them in the key value store
  */
}

grpc::Status Service::read(bytes chirp_id) {
  /* TODO:
    - Read chirp
  */
}

grpc::Status Service::monitor(const std::string &user) {
  /* TODO:
    - Make request to backend for all followers of user
    - Make request for all chirps of all followers. 
    - Update user-to-follow w/ user that is now following them in the key value store
  */
}


