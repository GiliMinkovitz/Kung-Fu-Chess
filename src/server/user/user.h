#pragma once

#include "server/user/user_id.h"

#include <string>

namespace kfc {

class User {
public:
    User(UserId id, std::string username);

    [[nodiscard]] UserId id() const noexcept;
    [[nodiscard]] const std::string& username() const noexcept;

private:
    UserId id_;
    std::string username_;
};

}  // namespace kfc
