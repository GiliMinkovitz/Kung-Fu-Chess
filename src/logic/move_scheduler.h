#pragma once

#include "core/piece.h"

#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

namespace kfc {

struct PendingMove {
    Piece piece;
    std::pair<std::size_t, std::size_t> start_pos;
    std::pair<std::size_t, std::size_t> end_pos;
    std::int64_t arrival_time = 0;
};

struct JumpState {
    Piece piece;
    std::pair<std::size_t, std::size_t> cell;
    std::int64_t arrival_time = 0;
};

class BoardModel;
class CollisionResolver;
struct GameRules;
struct ArrivingPieceInfo;

class MoveScheduler {
public:
    [[nodiscard]] std::int64_t clock_ms() const noexcept { return clock_ms_; }

    void add_clock(std::int64_t ms) noexcept { clock_ms_ += ms; }
    void for_each_pending_due(const std::function<void(const PendingMove&)>& fn);
    [[nodiscard]] bool check_for_jump_capture(
        const CollisionResolver& resolver, BoardModel& board, const GameRules& rules,
        const std::pair<std::size_t, std::size_t>& target_cell,
        const ArrivingPieceInfo& arriving_piece_info, bool& game_over) const;

    [[nodiscard]] bool is_piece_moving(std::size_t row, std::size_t col) const;
    [[nodiscard]] bool is_piece_jumping(std::size_t row, std::size_t col) const;
    [[nodiscard]] bool is_same_color_destination_claimed(
        char color, const std::pair<std::size_t, std::size_t>& end_pos) const;
    [[nodiscard]] bool conflicts_with_opposite_color_move(char moving_color,
                                                        const PendingMove& proposed) const;

    void schedule_move(PendingMove move);
    void schedule_jump(JumpState jump);
    void expire_jumps();

private:
    std::vector<PendingMove> pending_moves_;
    std::vector<JumpState> active_jumps_;
    std::int64_t clock_ms_ = 0;
};

}  // namespace kfc
