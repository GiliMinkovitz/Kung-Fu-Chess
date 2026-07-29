#pragma once

#include "model/piece.h"
#include "server/room/room_id.h"

#include <string>

namespace kfc {

struct GameRedirectInfo {
    std::string endpoint;
    RoomId room_id;
    PieceColor side;
};

}  // namespace kfc
