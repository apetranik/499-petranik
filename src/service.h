#ifndef SRC_CHIRP_SERVICE_LAYER_H_
#define SRC_CHIRP_SERVICE_LAYER_H_

#include <bits/stdc++.h>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
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
  std::optional<chirp::User> validate_monitor_request(
      const std::string &username);
  // Clear cached chirps & remove user as monitoring from users theyre following
  void terminate_monitor(chirp::User &user,
                         std::vector<std::string> &followed_by_user,
                         const std::string &username);
  // Cindy's Implementation for streaming chirps with hashtags
  std::vector<chirp::Chirp> stream(const std::string hashtag,
                                   std::time_t seconds,
                                   int64_t microseconds_since_epoch,
                                   std::set<std::string> &chirp_sent);
  // Test Stream function. This function is only called for testing purposes.
  // This class is an intermediatary to the actual GRPC call. This is also a
  // class that is used for testing. So the way the class is designed right now,
  // this function is called to test the stream function.
  std::vector<chirp::Chirp> TestStream(const std::string hashtag);
  // This function is called in chirp() to determine if the text consist of
  // an hashtag. If so, save that chirp into a proto Hashtags. This is public so
  // It can be tested.
  std::string CheckIfHaveHashtag(const std::string &text);
  // This function is a helper function to find multiple hashtags in a chirp, if
  // exists
  std::vector<std::string> CheckIfHaveMultipleHashtags(const std::string &text);
  // Helper function to set timestamp given variable containers, public because
  // its called by another class as well
  void SetTimeStamp(std::time_t &seconds, int64_t &microseconds_since_epoch);

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
  // Helper function to add hashtag to database. Figure out if this hashtag word
  // exist in database already, if so, just append to that proto, if not make a
  // new hashtag proto. Takes in a chirp to add to the database and the hashtag
  // its associated with it
  void AddHashtagToDatabase(const chirp::Chirp &hash,
                            const std::string &hashtagword);
  // Helper function to copy a chirp_to_copy into empty_chirp.
  void CopyChirp(chirp::Chirp *empty_chirp, chirp::Chirp chirp_to_copy);
  // Empty string constant
  const std::string kEmptyKey = "[empty_key]";
};

#endif
