#pragma once

#include "server/game/i_game_host.h"
#include "server/room/room_manager.h"

namespace kfc {

class LocalGameHost : public IGameHost {
public:
    explicit LocalGameHost(RoomManager& room_manager);

    RoomId create_room(const GamePlayer& white, const GamePlayer& black,
                       std::optional<int> db_game_id) override;

private:
    RoomManager& room_manager_;
};

}  // namespace kfc
