#include "server/room/room_manager.h"

namespace kfc {

RoomManager::RoomManager(BoardModel default_board) : default_board_(std::move(default_board)) {}

RoomId RoomManager::create_room() {
    const RoomId id = next_id_++;
    rooms_.emplace(id, std::make_unique<Room>(id, default_board_));
    return id;
}

void RoomManager::remove_room(RoomId id) {
    rooms_.erase(id);
}

Room* RoomManager::find_room(RoomId id) noexcept {
    const auto it = rooms_.find(id);
    return it != rooms_.end() ? it->second.get() : nullptr;
}

const Room* RoomManager::find_room(RoomId id) const noexcept {
    const auto it = rooms_.find(id);
    return it != rooms_.end() ? it->second.get() : nullptr;
}

void RoomManager::tick_all(std::int64_t delta_ms) {
    for (auto& [id, room] : rooms_) {
        if (room->active() && !room->is_game_over()) {
            room->tick(delta_ms);
        }
    }
}

void RoomManager::remove_inactive_rooms() {
    for (auto it = rooms_.begin(); it != rooms_.end();) {
        if (!it->second->active()) {
            it = rooms_.erase(it);
        } else {
            ++it;
        }
    }
}

std::vector<Room*> RoomManager::active_rooms() noexcept {
    std::vector<Room*> result;
    result.reserve(rooms_.size());
    for (auto& [id, room] : rooms_) {
        if (room->active()) {
            result.push_back(room.get());
        }
    }
    return result;
}

std::vector<const Room*> RoomManager::active_rooms() const noexcept {
    std::vector<const Room*> result;
    result.reserve(rooms_.size());
    for (const auto& [id, room] : rooms_) {
        if (room->active()) {
            result.push_back(room.get());
        }
    }
    return result;
}

}  // namespace kfc
