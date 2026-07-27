#pragma once

#include "server/user/user_id.h"

#include <string>

namespace kfc {

struct UserCredentials {
    UserId id;
    std::string username;
    int rating;
    std::string password_hash;
};

}  // namespace kfc
