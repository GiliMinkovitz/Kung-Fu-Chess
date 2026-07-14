#include "move_scheduler.h"

#include "../model/board_model.h"
#include "collision_resolver.h"
#include "../rules/game_rules.h"

namespace kfc {

bool MoveScheduler::is_piece_moving(uint64_t current_time_ms, std::size_t row,
                                    std::size_t col) const {
    const auto clock_ms = static_cast<std::int64_t>(current_time_ms);
    for (const PendingMove& move : pending_moves_) {
        if (clock_ms < move.arrival_time && move.start_pos.first == row &&
            move.start_pos.second == col) {
            return true;
        }
    }
    return false;
}

bool MoveScheduler::is_piece_jumping(uint64_t current_time_ms, std::size_t row,
                                     std::size_t col) const {
    const auto clock_ms = static_cast<std::int64_t>(current_time_ms);
    for (const JumpState& jump : active_jumps_) {
        if (clock_ms < jump.arrival_time && jump.cell.first == row &&
            jump.cell.second == col) {
            return true;
        }
    }
    return false;
}

bool MoveScheduler::is_same_color_destination_claimed(
    uint64_t current_time_ms, PieceColor color,
    const std::pair<std::size_t, std::size_t>& end_pos) const {
    return CollisionResolver::is_same_color_destination_claimed(
        pending_moves_, static_cast<std::int64_t>(current_time_ms), color, end_pos);
}

bool MoveScheduler::conflicts_with_opposite_color_move(uint64_t current_time_ms,
                                                     PieceColor moving_color,
                                                     const PendingMove& proposed) const {
    return CollisionResolver::conflicts_with_opposite_color_move(
        pending_moves_, static_cast<std::int64_t>(current_time_ms), moving_color, proposed);
}

void MoveScheduler::for_each_pending_due(uint64_t current_time_ms,
                                         const std::function<void(const PendingMove&)>& fn) {
    const auto clock_ms = static_cast<std::int64_t>(current_time_ms);
    std::vector<PendingMove> still_pending;
    still_pending.reserve(pending_moves_.size());

    for (PendingMove& move : pending_moves_) {
        if (clock_ms < move.arrival_time) {
            still_pending.push_back(std::move(move));
        } else {
            fn(move);
        }
    }

    pending_moves_ = std::move(still_pending);
}

bool MoveScheduler::check_for_jump_capture(
    uint64_t current_time_ms, const CollisionResolver& resolver, BoardModel& board,
    const GameRules& rules, const std::pair<std::size_t, std::size_t>& target_cell,
    const ArrivingPieceInfo& arriving_piece_info, bool& game_over) const {
    return resolver.check_for_jump_capture(board, rules, static_cast<std::int64_t>(current_time_ms),
                                           active_jumps_, target_cell, arriving_piece_info,
                                           game_over);
}

void MoveScheduler::schedule_move(PendingMove move) {
    pending_moves_.push_back(std::move(move));
}

void MoveScheduler::schedule_jump(JumpState jump) {
    active_jumps_.push_back(std::move(jump));
}

void MoveScheduler::expire_jumps(uint64_t current_time_ms) {
    const auto clock_ms = static_cast<std::int64_t>(current_time_ms);
    std::vector<JumpState> still_jumping;
    still_jumping.reserve(active_jumps_.size());

    for (JumpState& jump : active_jumps_) {
        if (clock_ms < jump.arrival_time) {
            still_jumping.push_back(std::move(jump));
        }
    }

    active_jumps_ = std::move(still_jumping);
}

}  // namespace kfc
