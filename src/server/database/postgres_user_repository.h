#pragma once

#include "server/database/i_user_repository.h"

#include "database/postgres_connection.h"

#include <optional>

namespace kfc {

// Requires players/games tables created by PostgresConnection::initialize_schema().

class PostgresUserRepository final : public IUserRepository {
public:
    explicit PostgresUserRepository(PostgresConnection& database);

    [[nodiscard]] UserId create_user(std::string username) override;
    [[nodiscard]] UserId create_user(UserId id, std::string username) override;
    [[nodiscard]] const User* find_by_id(UserId id) const noexcept override;
    [[nodiscard]] const User* find_by_username(const std::string& username) const noexcept override;
    [[nodiscard]] std::optional<UserCredentials> find_credentials_by_username(
        const std::string& username) const override;
    [[nodiscard]] std::optional<Player> find_profile_by_id(UserId id) const override;
    [[nodiscard]] std::optional<Player> create_user_with_password(
        std::string username, int rating, std::string password_hash) override;
    [[nodiscard]] bool update_rating(UserId id, int rating) override;

private:
    [[nodiscard]] const User* cache_user(UserId id, const std::string& username) const noexcept;

    PostgresConnection& database_;
    mutable std::optional<User> cached_user_;
};

}  // namespace kfc
