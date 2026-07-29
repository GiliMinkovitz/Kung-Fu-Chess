#pragma once

#include "server/game/i_game_host.h"

namespace kfc {

class RoomManager;

class LocalGameHost : public IGameHost {
public:
    explicit LocalGameHost(RoomManager& room_manager);

    [[nodiscard]] RoomId create_room() override;
    void activate_room(RoomId room_id, const GamePlayer& white,
                       const GamePlayer& black) override;
    void set_db_game_id(RoomId room_id, int db_game_id) override;

private:
    RoomManager& room_manager_;
};

}  // namespace kfc
