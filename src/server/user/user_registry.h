#pragma once

#include "server/user/user.h"

#include <optional>
#include <string>
#include <unordered_map>

namespace kfc {

class UserRegistry {
public:
    [[nodiscard]] UserId register_user(UserId id, std::string username);
    [[nodiscard]] UserId create_user(std::string username);
    [[nodiscard]] const User* find(UserId id) const noexcept;
    [[nodiscard]] const User* find_by_username(const std::string& username) const noexcept;

private:
    UserId next_id_ = 1;
    std::unordered_map<UserId, User> users_;
    std::unordered_map<std::string, UserId> by_username_;
};

}  // namespace kfc
