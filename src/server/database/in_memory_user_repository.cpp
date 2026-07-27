#include "server/database/in_memory_user_repository.h"

namespace kfc {

UserId InMemoryUserRepository::create_user(UserId id, std::string username) {
    users_.insert_or_assign(id, User(id, username));
    by_username_[username] = id;
    return id;
}

UserId InMemoryUserRepository::create_user(std::string username) {
    const UserId id = next_id_++;
    return create_user(id, std::move(username));
}

const User* InMemoryUserRepository::find_by_id(const UserId id) const noexcept {
    const auto it = users_.find(id);
    return it != users_.end() ? &it->second : nullptr;
}

const User* InMemoryUserRepository::find_by_username(const std::string& username) const noexcept {
    const auto it = by_username_.find(username);
    if (it == by_username_.end()) {
        return nullptr;
    }
    return find_by_id(it->second);
}

}  // namespace kfc
