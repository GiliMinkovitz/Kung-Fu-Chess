#include "server/user/user.h"

namespace kfc {

User::User(UserId id, std::string username) : id_(id), username_(std::move(username)) {}

UserId User::id() const noexcept {
    return id_;
}

const std::string& User::username() const noexcept {
    return username_;
}

}  // namespace kfc
