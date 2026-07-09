#pragma once

#include "core/board_model.h"
#include "logic/game_rules.h"
#include "logic/move_scheduler.h"

#include <utility>

namespace kfc {

struct ArrivingPieceInfo {
    Piece piece;
    std::pair<std::size_t, std::size_t> start_pos;
    std::pair<std::size_t, std::size_t> end_pos;
};

class CollisionResolver {
public:
    [[nodiscard]] static bool has_common_route(const PendingMove& a, const PendingMove& b);
    [[nodiscard]] static bool conflicts_with_opposite_color_move(
        const std::vector<PendingMove>& pending_moves, std::int64_t clock_ms, char moving_color,
        const PendingMove& proposed);
    [[nodiscard]] static bool is_same_color_destination_claimed(
        const std::vector<PendingMove>& pending_moves, std::int64_t clock_ms, char moving_color,
        const std::pair<std::size_t, std::size_t>& end_pos);

    [[nodiscard]] bool check_for_jump_capture(
        BoardModel& board, const GameRules& rules, std::int64_t clock_ms,
        const std::vector<JumpState>& active_jumps,
        const std::pair<std::size_t, std::size_t>& target_cell,
        const ArrivingPieceInfo& arriving_piece_info, bool& game_over) const;
};

}  // namespace kfc
