#pragma once

#include <string>
#include <unordered_set>

namespace kfc {

class SessionRegistry {
public:
    [[nodiscard]] bool is_online(const std::string& username) const;
    bool register_session(const std::string& username);
    void unregister_session(const std::string& username);

private:
    std::unordered_set<std::string> online_usernames_;
};

}  // namespace kfc
