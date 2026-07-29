#pragma once

#include "server/room/game_player.h"
#include "server/room/room_id.h"

namespace kfc {

class IGameHost {
public:
    virtual ~IGameHost() = default;

    [[nodiscard]] virtual RoomId create_room() = 0;
    virtual void activate_room(RoomId room_id, const GamePlayer& white,
                               const GamePlayer& black) = 0;
    virtual void set_db_game_id(RoomId room_id, int db_game_id) = 0;
};

}  // namespace kfc
