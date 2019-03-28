#ifndef SRC_CHIRP_KEY_VALUE_STORE_INTERFACE_H_
#define SRC_CHIRP_KEY_VALUE_STORE_INTERFACE_H_
#include <optional>
#include <string>

// Interface for KeyValueStore that communicates w/ Service Layer.
class KeyValueStoreInterface {
 public:
  virtual ~KeyValueStoreInterface() = 0;
  // get value from key-value store using key.
  // return true if successful
  virtual std::optional<std::string> Get(const std::string &key) = 0;
  // puts new key,value or updates value in store
  // return true in successful
  virtual bool Put(const std::string &key, const std::string &value) = 0;
  // deletes key,value from store
  // return true in successful
  virtual bool Del(const std::string &key) = 0;
};
#endif
