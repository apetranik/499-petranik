#include <iostream>

// The middle service layer that handles calls from the command line tool and acceses the backend key-value store
class Service {
  public:
   // Posts a new chirp (optionally as a reply)
   message chirp(const std::string &user, const std::string &text, bytes chirp_id);
   // Registers the given non-blank username
   message registerUser(const std::string &user);
   // Starts following a given user
   message follow(const std::string &user, const std::string &user_to_follow);
   // Reads a chirp thread from the given id
   message read(bytes chirp_id);
   // Streams chirps from all followed users
   message monitor(const std::string &user);
};
	