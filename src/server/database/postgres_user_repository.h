#pragma once

#include "server/database/i_user_repository.h"

#include "database/postgres_connection.h"

#include <optional>

namespace kfc {

// Expected PostgreSQL schema (apply via external migration tooling; not created here):
//
// CREATE TABLE players (
//     id SERIAL PRIMARY KEY,
//     username TEXT NOT NULL UNIQUE,
//     rating INTEGER NOT NULL DEFAULT 1000,
//     password_hash TEXT
// );
//
// CREATE TABLE games (
//     id SERIAL PRIMARY KEY,
//     white_player_id INTEGER,
//     black_player_id INTEGER,
//     winner_id INTEGER,
//     status TEXT NOT NULL,
//     created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
// );

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
