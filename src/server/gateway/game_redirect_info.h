#pragma once

#include "model/piece.h"
#include "server/room/room_id.h"

#include <string>

namespace kfc {

struct GameRedirectInfo {
    RoomId room_id;
    std::string server_id;
    std::string endpoint;
    PieceColor side;
};

}  // namespace kfc
