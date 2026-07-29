#pragma once

#include "server/room/game_player.h"
#include "server/room/room_id.h"

#include <optional>

namespace kfc {

class IGameHost {
public:
    virtual ~IGameHost() = default;

    virtual RoomId create_room(const GamePlayer& white, const GamePlayer& black,
                               std::optional<int> db_game_id) = 0;
};

}  // namespace kfc
