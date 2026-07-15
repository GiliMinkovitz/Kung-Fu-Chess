#pragma once

#include "../model/board_model.h"
#include "../rules/game_rules.h"
#include "move_scheduler.h"

#include <utility>

namespace kfc {

struct ArrivingPieceInfo {
    Piece::Id piece_id = Piece::kInvalidId;
    std::pair<std::size_t, std::size_t> start_pos;
    std::pair<std::size_t, std::size_t> end_pos;
};

// Kung Fu Chess collision domain: route overlap, same-color destination claims, and
// arrival resolution (jump capture vs normal capture). Extracted from RealTimeArbiter
// so conflict logic is unit-testable without driving the full clock/settlement loop.
class CollisionResolver {
public:
    [[nodiscard]] static bool has_common_route(const PendingMove& a, const PendingMove& b);
    [[nodiscard]] static bool conflicts_with_opposite_color_move(
        const std::vector<PendingMove>& pending_moves, std::int64_t clock_ms,
        PieceColor moving_color, const PendingMove& proposed);
    [[nodiscard]] static bool is_same_color_destination_claimed(
        const std::vector<PendingMove>& pending_moves, std::int64_t clock_ms,
        PieceColor moving_color, const std::pair<std::size_t, std::size_t>& end_pos);

    // Returns true if the arrival was resolved as a capture (jump or occupant);
    // false leaves normal placement to the caller (RealTimeArbiter).
    [[nodiscard]] bool check_for_jump_capture(
        BoardModel& board, const GameRules& rules, std::int64_t clock_ms,
        const std::vector<JumpState>& active_jumps,
        const std::pair<std::size_t, std::size_t>& target_cell,
        const ArrivingPieceInfo& arriving_piece_info, bool& game_over) const;
};

}  // namespace kfc
