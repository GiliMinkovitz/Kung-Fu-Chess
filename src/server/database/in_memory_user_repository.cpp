#include "server/database/in_memory_user_repository.h"

namespace kfc {

UserId InMemoryUserRepository::create_user(UserId id, std::string username) {
    users_.insert_or_assign(id, StoredUser{User(id, username), 1000, ""});
    by_username_[username] = id;
    return id;
}

UserId InMemoryUserRepository::create_user(std::string username) {
    const UserId id = next_id_++;
    return create_user(id, std::move(username));
}

const User* InMemoryUserRepository::find_by_id(const UserId id) const noexcept {
    const auto it = users_.find(id);
    return it != users_.end() ? &it->second.user : nullptr;
}

const User* InMemoryUserRepository::find_by_username(const std::string& username) const noexcept {
    const auto it = by_username_.find(username);
    if (it == by_username_.end()) {
        return nullptr;
    }
    return find_by_id(it->second);
}

std::optional<UserCredentials> InMemoryUserRepository::find_credentials_by_username(
    const std::string& username) const {
    const User* user = find_by_username(username);
    if (user == nullptr) {
        return std::nullopt;
    }

    const StoredUser& stored = users_.at(user->id());
    return UserCredentials{user->id(), user->username(), stored.rating, stored.password_hash};
}

std::optional<Player> InMemoryUserRepository::find_profile_by_id(const UserId id) const {
    const auto it = users_.find(id);
    if (it == users_.end()) {
        return std::nullopt;
    }
    return Player(static_cast<int>(it->second.user.id()), it->second.user.username(),
                  it->second.rating);
}

std::optional<Player> InMemoryUserRepository::create_user_with_password(
    std::string username, int rating, std::string password_hash) {
    if (find_by_username(username) != nullptr) {
        return std::nullopt;
    }

    const UserId id = next_id_++;
    users_.insert_or_assign(id, StoredUser{User(id, username), rating, std::move(password_hash)});
    by_username_[username] = id;
    return Player(static_cast<int>(id), username, rating);
}

bool InMemoryUserRepository::update_rating(const UserId id, int rating) {
    const auto it = users_.find(id);
    if (it == users_.end()) {
        return false;
    }
    it->second.rating = rating;
    return true;
}

}  // namespace kfc
