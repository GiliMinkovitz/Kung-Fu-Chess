#pragma once

#include "server/user/user.h"
#include "server/user/user_id.h"

#include <string>

namespace kfc {

class IUserRepository {
public:
    virtual ~IUserRepository() = default;

    [[nodiscard]] virtual UserId create_user(std::string username) = 0;
    [[nodiscard]] virtual UserId create_user(UserId id, std::string username) = 0;
    [[nodiscard]] virtual const User* find_by_id(UserId id) const noexcept = 0;
    [[nodiscard]] virtual const User* find_by_username(const std::string& username) const noexcept = 0;
};

}  // namespace kfc
