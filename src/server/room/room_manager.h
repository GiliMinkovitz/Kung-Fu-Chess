#pragma once

#include "model/board_model.h"
#include "server/room/room.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace kfc {

class RoomManager {
public:
    explicit RoomManager(BoardModel default_board);

    [[nodiscard]] RoomId create_room();
    void remove_room(RoomId id);
    [[nodiscard]] Room* find_room(RoomId id) noexcept;
    [[nodiscard]] const Room* find_room(RoomId id) const noexcept;

    void tick_all(std::int64_t delta_ms);
    void remove_inactive_rooms();

    [[nodiscard]] std::vector<Room*> active_rooms() noexcept;
    [[nodiscard]] std::vector<const Room*> active_rooms() const noexcept;

private:
    BoardModel default_board_;
    RoomId next_id_ = 1;
    std::unordered_map<RoomId, std::unique_ptr<Room>> rooms_;
};

}  // namespace kfc
