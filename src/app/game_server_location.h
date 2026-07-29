#pragma once

#include "server/room/room_id.h"

#include <string>

namespace kfc {

struct GameServerLocation {
    std::string server_id;
    std::string endpoint;
    RoomId room_id;
};

}  // namespace kfc
