#include "key_value_store.h"
#include <string>
#include <iostream>
#include <optional>

KeyValueStore::KeyValueStore() : key_value_store_() {}

KeyValueStore::~KeyValueStore() {}

// TODO: make test for get + support concurrency
bool KeyValueStore::Get(const std::string &key, std::string *output_value) {
  auto it = key_value_store_.find(key);

  if (it != key_value_store_.end()) {
    *output_value = it->second;
    return true;
  }
  return false;
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
