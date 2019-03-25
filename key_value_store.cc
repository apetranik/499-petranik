#include "key_value_store.h"

KeyValueStore::KeyValueStore() : key_value_store_() {}

KeyValueStore::~KeyValueStore() {}

std::optional<std::string> KeyValueStore::Get(const std::string &key) {
  auto it = key_value_store_.find(key);

  if (it != key_value_store_.end()) {
    if (!it->second.empty()) {
      return it->second;
    }
  } else {
    return {};
  }
}

bool KeyValueStore::Put(const std::string &key, const std::string &value) {
  key_value_store_[key] = value;
  return true;
}

bool KeyValueStore::Del(const std::string &key) {
  auto success = key_value_store_.erase(key);
  return success;
}
