#pragma once

#include "server/database/i_user_repository.h"

#include <string>
#include <unordered_map>

namespace kfc {

class InMemoryUserRepository final : public IUserRepository {
public:
    [[nodiscard]] UserId create_user(std::string username) override;
    [[nodiscard]] UserId create_user(UserId id, std::string username) override;
    [[nodiscard]] const User* find_by_id(UserId id) const noexcept override;
    [[nodiscard]] const User* find_by_username(const std::string& username) const noexcept override;

private:
    UserId next_id_ = 1;
    std::unordered_map<UserId, User> users_;
    std::unordered_map<std::string, UserId> by_username_;
};

}  // namespace kfc
