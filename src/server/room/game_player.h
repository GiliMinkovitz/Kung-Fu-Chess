#pragma once

#include "model/piece.h"
#include "server/network/player_id.h"
#include "server/user/user_id.h"

namespace kfc {

struct GamePlayer {
    UserId user_id;
    PieceColor side;
    PlayerId player_id;
};

}  // namespace kfc
