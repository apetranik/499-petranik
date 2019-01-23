#include <iostream>
#include <grpcpp/grpcpp.h>

class Backend
{
  public:
    message put(const std::string &key, const std::string &value);
    message get(const std::string &key);
    message del(const std::string &key);
};
