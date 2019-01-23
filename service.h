#include <iostream>

class Service
{
  public:
    message chirp(const std::string &user, const std::string &text, int chirpID);
    message registerUser(const std::string &user);

    message follow(const std::string &user, const std::string &userToFollow);
    message read(int chirpID);
    message monitor(const std::string &user);
    message monitor(const std::string &user);
};
