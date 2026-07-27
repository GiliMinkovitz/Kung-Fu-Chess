#include "server/database/sqlite_user_repository.h"

namespace kfc {

SqliteUserRepository::SqliteUserRepository(SqliteDatabase& database)
    : player_repository_(database) {}

UserId SqliteUserRepository::create_user(std::string username) {
    if (const auto created = player_repository_.create_player(username)) {
        return static_cast<UserId>(created->id());
    }
    return 0;
}

UserId SqliteUserRepository::create_user(const UserId id, std::string username) {
    if (player_repository_.find_by_id(static_cast<int>(id)).has_value()) {
        return id;
    }
    if (const auto created = player_repository_.create_player(std::move(username))) {
        return static_cast<UserId>(created->id());
    }
    return id;
}

const User* SqliteUserRepository::cache_user(const UserId id,
                                             const std::string& username) const noexcept {
    cached_user_ = User(id, username);
    return &*cached_user_;
}

const User* SqliteUserRepository::find_by_id(const UserId id) const noexcept {
    if (const auto profile = player_repository_.find_by_id(static_cast<int>(id))) {
        return cache_user(id, profile->username());
    }
    return nullptr;
}

const User* SqliteUserRepository::find_by_username(const std::string& username) const noexcept {
    if (const auto profile = player_repository_.find_by_username(username)) {
        return cache_user(static_cast<UserId>(profile->id()), profile->username());
    }
    return nullptr;
}

std::optional<UserCredentials> SqliteUserRepository::find_credentials_by_username(
    const std::string& username) const {
    if (const auto credentials = player_repository_.find_credentials_by_username(username)) {
        return UserCredentials{
            static_cast<UserId>(credentials->id),
            credentials->username,
            credentials->rating,
            credentials->password_hash,
        };
    }
    return std::nullopt;
}

std::optional<Player> SqliteUserRepository::find_profile_by_id(const UserId id) const {
    return player_repository_.find_by_id(static_cast<int>(id));
}

std::optional<Player> SqliteUserRepository::create_user_with_password(
    std::string username, int rating, std::string password_hash) {
    return player_repository_.create_player(username, rating, password_hash);
}

bool SqliteUserRepository::update_rating(const UserId id, int rating) {
    return player_repository_.update_rating(static_cast<int>(id), rating);
}

}  // namespace kfc
