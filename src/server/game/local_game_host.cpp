#include "server/game/local_game_host.h"

#include "server/room/room.h"

namespace kfc {

LocalGameHost::LocalGameHost(RoomManager& room_manager) : room_manager_(room_manager) {}

RoomId LocalGameHost::create_room(const GamePlayer& white, const GamePlayer& black,
                                  std::optional<int> db_game_id) {
    const RoomId room_id = room_manager_.create_room();
    Room* room = room_manager_.find_room(room_id);
    room->activate(white, black);
    if (db_game_id.has_value()) {
        room->set_db_game_id(*db_game_id);
    }
    return room_id;
}

}  // namespace kfc
