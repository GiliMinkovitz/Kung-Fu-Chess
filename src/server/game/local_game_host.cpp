#include "server/game/local_game_host.h"

#include "server/room/room.h"
#include "server/room/room_manager.h"

namespace kfc {

LocalGameHost::LocalGameHost(RoomManager& room_manager) : room_manager_(room_manager) {}

RoomId LocalGameHost::create_room() {
    return room_manager_.create_room();
}

void LocalGameHost::activate_room(RoomId room_id, const GamePlayer& white,
                                  const GamePlayer& black) {
    Room* room = room_manager_.find_room(room_id);
    room->activate(white, black);
}

void LocalGameHost::set_db_game_id(RoomId room_id, int db_game_id) {
    Room* room = room_manager_.find_room(room_id);
    room->set_db_game_id(db_game_id);
}

}  // namespace kfc
