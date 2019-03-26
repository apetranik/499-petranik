#ifndef CHIRP_KEY_VALUE_STORE_H_
#define CHIRP_KEY_VALUE_STORE_H_

#include <iostream>
#include <map>
#include <optional>
#include <string>

// the key-value data structure for storing chirp data
// Has GET, PUT, DEL functionality
class KeyValueStore {
 public:
  KeyValueStore();
  virtual ~KeyValueStore();
  // get value from key-value store using key.
  // return true if successful
  std::optional<std::string> Get(const std::string &key);
  // puts new key,value or updates value in store
  // return true in successful
  bool Put(const std::string &key, const std::string &value);
  // deletes key,value from store
  // return true in successful
  bool Del(const std::string &key);

 private:
  // data structure.
  std::map<std::string, std::string> key_value_store_;
};

#endif  // CHIRP_SRC_BACKEND_DATA_STRUCTURE_H_
