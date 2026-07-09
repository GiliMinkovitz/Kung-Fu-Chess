#include "logic/move_scheduler.h"

#include "core/board_model.h"
#include "logic/collision_resolver.h"
#include "logic/game_rules.h"

namespace kfc {

bool MoveScheduler::is_piece_moving(std::size_t row, std::size_t col) const {
    for (const PendingMove& move : pending_moves_) {
        if (clock_ms_ < move.arrival_time && move.start_pos.first == row &&
            move.start_pos.second == col) {
            return true;
        }
    }
    return false;
}

bool MoveScheduler::is_piece_jumping(std::size_t row, std::size_t col) const {
    for (const JumpState& jump : active_jumps_) {
        if (clock_ms_ < jump.arrival_time && jump.cell.first == row &&
            jump.cell.second == col) {
            return true;
        }
    }
    return false;
}

bool MoveScheduler::is_same_color_destination_claimed(
    PieceColor color, const std::pair<std::size_t, std::size_t>& end_pos) const {
    return CollisionResolver::is_same_color_destination_claimed(pending_moves_, clock_ms_, color,
                                                                end_pos);
}

bool MoveScheduler::conflicts_with_opposite_color_move(PieceColor moving_color,
                                                       const PendingMove& proposed) const {
    return CollisionResolver::conflicts_with_opposite_color_move(pending_moves_, clock_ms_,
                                                                 moving_color, proposed);
}

void MoveScheduler::for_each_pending_due(const std::function<void(const PendingMove&)>& fn) {
    std::vector<PendingMove> still_pending;
    still_pending.reserve(pending_moves_.size());

    for (PendingMove& move : pending_moves_) {
        if (clock_ms_ < move.arrival_time) {
            still_pending.push_back(std::move(move));
        } else {
            fn(move);
        }
    }

    pending_moves_ = std::move(still_pending);
}

bool MoveScheduler::check_for_jump_capture(
    const CollisionResolver& resolver, BoardModel& board, const GameRules& rules,
    const std::pair<std::size_t, std::size_t>& target_cell,
    const ArrivingPieceInfo& arriving_piece_info, bool& game_over) const {
    return resolver.check_for_jump_capture(board, rules, clock_ms_, active_jumps_, target_cell,
                                           arriving_piece_info, game_over);
}

void MoveScheduler::schedule_move(PendingMove move) {
    pending_moves_.push_back(std::move(move));
}

void MoveScheduler::schedule_jump(JumpState jump) {
    active_jumps_.push_back(std::move(jump));
}

void MoveScheduler::expire_jumps() {
    std::vector<JumpState> still_jumping;
    still_jumping.reserve(active_jumps_.size());

    for (JumpState& jump : active_jumps_) {
        if (clock_ms_ < jump.arrival_time) {
            still_jumping.push_back(std::move(jump));
        }
    }

    active_jumps_ = std::move(still_jumping);
}

}  // namespace kfc
