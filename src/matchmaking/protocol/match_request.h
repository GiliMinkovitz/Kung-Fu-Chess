#pragma once

#include "server/network/player_id.h"
#include "server/user/user_id.h"

#include <string>

namespace kfc::matchmaking {

struct MatchRequest {
    PlayerId player_id = 0;
    UserId user_id = 0;
    int elo = 0;
    std::string region;
};

}  // namespace kfc::matchmaking
