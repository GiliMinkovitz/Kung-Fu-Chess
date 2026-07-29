#pragma once

#include "server/game_server.h"
#include "server/room/room.h"
#include "server/user/user.h"

#include <string>

namespace kfc::test {

[[nodiscard]] inline Room* find_room_for_player(GameServer& server, const std::string& username) {
    const kfc::User* user = server.user_repository().find_by_username(username);
    if (user == nullptr) {
        return nullptr;
    }

    for (Room* room : server.room_manager().active_rooms()) {
        if (const kfc::GamePlayer* white = room->white_player()) {
            if (white->user_id == user->id()) {
                return room;
            }
        }
        if (const kfc::GamePlayer* black = room->black_player()) {
            if (black->user_id == user->id()) {
                return room;
            }
        }
    }
    return nullptr;
}

}  // namespace kfc::test
