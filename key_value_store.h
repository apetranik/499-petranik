#include <map>
#include <string>
#include <optional>

// the key-value data structure for storing chirp data
// Has GET, PUT, DEL functionality
class BackendDataStructure {
 public:
  KeyValueStore();
  virtual ~KeyValueStore();

  // get value from key-value store using key. 
  // return true if successful
  bool Get(const std::string &key, std::string *reply);

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