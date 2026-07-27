#include "server/user/user_registry.h"

namespace kfc {

UserId UserRegistry::register_user(UserId id, std::string username) {
    users_.insert_or_assign(id, User(id, username));
    by_username_[username] = id;
    return id;
}

UserId UserRegistry::create_user(std::string username) {
    const UserId id = next_id_++;
    register_user(id, std::move(username));
    return id;
}

const User* UserRegistry::find(const UserId id) const noexcept {
    const auto it = users_.find(id);
    return it != users_.end() ? &it->second : nullptr;
}

const User* UserRegistry::find_by_username(const std::string& username) const noexcept {
    const auto it = by_username_.find(username);
    if (it == by_username_.end()) {
        return nullptr;
    }
    return find(it->second);
}

}  // namespace kfc
