#pragma once

#include "server/database/user_credentials.h"
#include "server/player.h"
#include "server/user/user.h"
#include "server/user/user_id.h"

#include <optional>
#include <string>

namespace kfc {

class IUserRepository {
public:
    virtual ~IUserRepository() = default;

    [[nodiscard]] virtual UserId create_user(std::string username) = 0;
    [[nodiscard]] virtual UserId create_user(UserId id, std::string username) = 0;
    [[nodiscard]] virtual const User* find_by_id(UserId id) const noexcept = 0;
    [[nodiscard]] virtual const User* find_by_username(const std::string& username) const noexcept = 0;
    [[nodiscard]] virtual std::optional<UserCredentials> find_credentials_by_username(
        const std::string& username) const = 0;
    [[nodiscard]] virtual std::optional<Player> find_profile_by_id(UserId id) const = 0;
    [[nodiscard]] virtual std::optional<Player> create_user_with_password(
        std::string username, int rating, std::string password_hash) = 0;
    [[nodiscard]] virtual bool update_rating(UserId id, int rating) = 0;
};

}  // namespace kfc
