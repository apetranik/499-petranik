#include "key_value_store.h"

KeyValueStore::KeyValueStore() : key_value_store_() {
  key_value_store_["num_chirps"] = "0";  // initalize to 0
}

KeyValueStore::~KeyValueStore() {}

std::optional<std::string> KeyValueStore::Get(const std::string &key) {
  auto it = key_value_store_.find(key);

  if (it != key_value_store_.end()) {
    return it->second;
  } else {
    return {};
  }
}

bool KeyValueStore::Put(const std::string &key, const std::string &value) {
  if (key.empty()) {
    return false;
  }
  key_value_store_[key] = value;
  return true;
}

bool KeyValueStore::Del(const std::string &key) {
  auto success = key_value_store_.erase(key);
  return success;
}
