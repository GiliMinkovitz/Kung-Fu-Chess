#pragma once

#include "logic/collision_resolver.h"
#include "logic/move_scheduler.h"

#include <cstdint>
#include <utility>

namespace kfc {

class BoardModel;
struct GameRules;

class RealTimeArbiter {
public:
    void update_time(std::int64_t ms, BoardModel& board, const GameRules& rules, bool& game_over);
    void settle_pending_moves(BoardModel& board, const GameRules& rules, bool& game_over);

    [[nodiscard]] std::int64_t clock_ms() const noexcept { return clock_ms_; }
    [[nodiscard]] bool is_piece_moving(std::size_t row, std::size_t col) const;
    [[nodiscard]] bool is_piece_jumping(std::size_t row, std::size_t col) const;
    [[nodiscard]] bool is_same_color_destination_claimed(
        PieceColor color, const std::pair<std::size_t, std::size_t>& end_pos) const;
    [[nodiscard]] bool conflicts_with_opposite_color_move(PieceColor moving_color,
                                                          const PendingMove& proposed) const;

    void schedule_move(PendingMove move);
    void schedule_jump(JumpState jump);

private:
    MoveScheduler scheduler_;
    CollisionResolver collision_resolver_;
    std::int64_t clock_ms_ = 0;
};

}  // namespace kfc
