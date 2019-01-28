#include <service.h>

#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>


// Accepts requests from command line and calls the backend key-value store to respond to requests

// Posts a new chirp (optionally as a reply)
grpc::Status Service::chirp(const std::string &user, const std::string &text, bytes chirp_id) {
  /* TODO:
    - Create a chirp
    - Send it to backend to be stored
    - send status back
  */
}
// Registers the given non-blank username
grpc::Status Service::registerUser(const std::string &user) {
  /* TODO:
    - Create a new user
    - Save it to backend to be stored
    - send status back
  */
}
// Starts following a given user
grpc::Status Service::follow(const std::string &user, const std::string &user_to_follow) {
  /* TODO:
    - Update user with a new follower in backend
    - Update user-to-follow w/ user that is now following them in the key value store
  */

}
// Reads a chirp thread from the given id
grpc::Status Service::read(bytes chirp_id) {
  /* TODO:
    - Read chirp
  */
}
// Streams chirps from all followed users
grpc::Status Service::monitor(const std::string &user) {
  /* TODO:
    - Make request to backend for all followers of user
    - Make request for all chirps of all followers. 
    - Update user-to-follow w/ user that is now following them in the key value store
  */
}


