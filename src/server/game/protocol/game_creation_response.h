#pragma once

#include "server/room/room_id.h"

#include <optional>
#include <string>

namespace kfc {

struct GameCreationResponse {
    RoomId room_id;
    std::string game_server_id;
    std::optional<std::string> endpoint;
};

}  // namespace kfc
