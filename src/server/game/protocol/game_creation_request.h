#pragma once

#include "server/user/user_id.h"

#include <optional>

namespace kfc {

struct GameCreationRequest {
    UserId white_user_id;
    UserId black_user_id;
    std::optional<int> db_game_id;
};

}  // namespace kfc
