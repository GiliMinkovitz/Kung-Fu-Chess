#include "server/session_registry.h"

namespace kfc {

bool SessionRegistry::is_online(const std::string& username) const {
    return online_usernames_.count(username) > 0;
}

bool SessionRegistry::register_session(const std::string& username) {
    if (username.empty() || is_online(username)) {
        return false;
    }
    online_usernames_.insert(username);
    return true;
}

void SessionRegistry::unregister_session(const std::string& username) {
    online_usernames_.erase(username);
}

}  // namespace kfc
