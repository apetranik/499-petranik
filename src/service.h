#ifndef SRC_CHIRP_SERVICE_LAYER_H_
#define SRC_CHIRP_SERVICE_LAYER_H_

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <glog/logging.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/util/time_util.h>
#include <grpcpp/grpcpp.h>

#include "../data_storage_types.grpc.pb.h"
#include "../service.grpc.pb.h"

#include "backend_client.h"
#include "key_value_store.h"
#define LOG_DIR "../logs/"
// A backend service to receive requests from client (command line)
class ServiceLayer final {
public:
  ServiceLayer(KeyValueStoreInterface *kvs_connection);
  // Constructs a RegisterRequest and sends to service layer thru grpc and
  // receives a RegisterReply back
  bool registeruser(const std::string &username);
  // Constructs a FollowRequest and sends to service layer thru grpc and
  // receives a FollowReply back
  std::string follow(const std::string &username,
                     const std::string &user_to_follow);
  // Constructs a ReadRequest and sends to service layer thru grpc and receives
  // a ReadReply back
  std::vector<chirp::Chirp> read(const std::string &chirp_id);
  // Constructs a MonitorRequest and sends to service layer thru grpc and
  // receives a stream MonitorReply back
  std::vector<chirp::Chirp> monitor(const std::string &user,
                                    std::vector<std::string> &followed_by_user,
                                    bool registeredAsMonitoring);
  // Constructs a ChirpRequest and sends to service layer thru grpc and receives
  // a ChirpReply back
  std::optional<chirp::Chirp> chirp(const std::string &user,
                                    const std::string &text,
                                    const std::string &parent_id);
  // checks existance of user and lets user know who they are following
  std::optional<chirp::User>
  validate_monitor_request(const std::string &username);
  // Clear cached chirps & remove user as monitoring from users theyre following
  void terminate_monitor(chirp::User &user,
                         std::vector<std::string> &followed_by_user,
                         const std::string &username);
  // // Cindy's Implementation for streaming chirps with hashtags
  std::vector<chirp::Chirp> stream(const std::string hashtag);
  // This function is called in chirp() to determine if the text consist of an
  // hashtag. If so, save that chirp into a proto Hashtags.
  std::string CheckIfHaveHashtag(const std::string &text);

private:
  // Helper function - performs DFS on chirp thread to collect all chirps and
  // replies to chirps
  void ThreadDFS(std::vector<chirp::Chirp> &chirp_thread, std::string chirp_id,
                 int depth);
  // Registers a user as "monitoring" with everyone they follow
  void register_monitoring_user(const std::string &username,
                                std::vector<std::string> &followed_by_user);

  // Interface that is used to make all service requests from key value store
  // thru grpc or directly to KVS
  KeyValueStoreInterface *backend_client_;
  // mutex used for concurrency
  std::mutex mutex_;
  // trim() takes trailing spaces off strings
  std::string trim(std::string &str);
  // helper function to add hashtag to database
  void AddHashtagToDatabase(const chirp::Chirp &hash,
                            const std::string &hashtagword);
};

#endif
