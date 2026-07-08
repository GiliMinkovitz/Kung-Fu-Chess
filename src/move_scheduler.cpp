#include "move_scheduler.h"

#include "collision_resolver.h"

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
    char color, const std::pair<std::size_t, std::size_t>& end_pos) const {
    return CollisionResolver::is_same_color_destination_claimed(pending_moves_, clock_ms_, color,
                                                                end_pos);
}

bool MoveScheduler::conflicts_with_opposite_color_move(char moving_color,
                                                       const PendingMove& proposed) const {
    return CollisionResolver::conflicts_with_opposite_color_move(pending_moves_, clock_ms_,
                                                                 moving_color, proposed);
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

void MoveScheduler::set_pending_moves(std::vector<PendingMove> moves) {
    pending_moves_ = std::move(moves);
}

}  // namespace kfc
