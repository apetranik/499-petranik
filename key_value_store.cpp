#include "key_value_store.h"

KeyValueStore::KeyValueStore() : key_value_store() {}

KeyValueStore::~KeyValueStore() {}

// TODO: make test for get + support concurrency
bool KeyValueStore::Get(const std::string &key, std::string *reply) {
  auto it = key_value_store.find(key);

  if (it != key_value_store.end()) {
    *reply = it->second;
    return true;
  }
  return false;
}

// TODO: Make test for put + support concurrency
bool KeyValueStore::Put(const std::string &key, const std::string &value) {
  key_value_store[key] = value;
  return true;
}

// TODO: Make test for delete + support concurrency
bool KeyValueStore::Delete(const std::string &key) {
  auto success = key_value_store.erase(key_to_del);
  return success;
}