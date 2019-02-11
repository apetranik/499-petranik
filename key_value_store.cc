#include "key_value_store.h"

#include <optional>
#include <functional>
#include <string>
#include <vector>
#include <iostream>

KeyValueStore::KeyValueStore() : key_value_store_() {}

KeyValueStore::~KeyValueStore() {}

// TODO: make test for get + support concurrency
std::optional<std::string> KeyValueStore::Get(const std::string &key) {
  auto it = key_value_store_.find(key);

  if (it != key_value_store_.end()) {
    if (!it->second.empty()) {
      return it->second;
    } 
  }
  else {
    return {};
  }
}

// TODO: Make test for put + support concurrency
bool KeyValueStore::Put(const std::string &key, const std::string &value) {
  key_value_store_[key] = value;
  return true;
}

// TODO: Make test for delete + support concurrency
bool KeyValueStore::Del(const std::string &key) {
  auto success = key_value_store_.erase(key);
  return success;
}
