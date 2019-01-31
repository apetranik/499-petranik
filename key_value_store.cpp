#include "key_value_store.h"

KeyValueStore::KeyValueStore() : key_value_store() {}

KeyValueStore::~KeyValueStore() {}

// TODO: make test for get + support concurrency
std::optional<std::string *> KeyValueStore::Get(const std::string &key) {
  auto it = key_value_store_.find(key);

  if (it != key_value_store_.end()) {
    return it->second;
  }
  return;
}

// TODO: Make test for put + support concurrency
bool KeyValueStore::Put(const std::string &key, const std::string &value) {
  key_value_store_[key] = value;
  return true;
}

// TODO: Make test for delete + support concurrency
bool KeyValueStore::Del(const std::string &key) {
  auto success = key_value_store_.erase(key_to_del);
  return success;
}