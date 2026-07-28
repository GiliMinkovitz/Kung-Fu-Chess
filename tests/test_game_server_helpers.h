#pragma once

#include "server/game_server.h"
#include "server/room/room.h"

#include <string>

namespace kfc::test {

[[nodiscard]] inline Room* find_room_for_player(GameServer& server, const std::string& username) {
    for (Room* room : server.room_manager().active_rooms()) {
        if (room->white_player() != nullptr && room->white_player()->username() == username) {
            return room;
        }
        if (room->black_player() != nullptr && room->black_player()->username() == username) {
            return room;
        }
    }
    return nullptr;
}

}  // namespace kfc::test
