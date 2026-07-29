#pragma once

#include "server/network/player_id.h"
#include "server/user/user_id.h"

#include <chrono>
#include <cstdint>

namespace kfc::matchmaking {

struct QueuedPlayer {
    PlayerId player_id = 0;
    UserId user_id = 0;
    int elo = 0;
    std::int64_t enqueue_epoch_seconds = 0;
};

}  // namespace kfc::matchmaking
