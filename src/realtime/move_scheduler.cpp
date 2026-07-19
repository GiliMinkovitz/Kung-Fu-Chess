#include "move_scheduler.h"

#include "../model/board_model.h"
#include "collision_resolver.h"
#include "../rules/game_rules.h"

#include <algorithm>

namespace kfc {

bool MoveScheduler::is_piece_moving(uint64_t current_time_ms, std::size_t row,
                                    std::size_t col) const {
    const auto clock_ms = static_cast<std::int64_t>(current_time_ms);
    // True when a pending move's logical start_pos matches this cell. The source cell may
    // already be empty on the board; use AnimationSnapshot for visual placement.
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

void MoveScheduler::for_each_in_flight_pending(
    uint64_t current_time_ms, const std::function<void(const PendingMove&)>& fn) const {
    const auto clock_ms = static_cast<std::int64_t>(current_time_ms);
    for (const PendingMove& move : pending_moves_) {
        if (clock_ms < move.arrival_time) {
            fn(move);
        }
    }
}

void MoveScheduler::cancel_pending_move(Piece::Id piece_id) {
    const auto it = std::find_if(pending_moves_.begin(), pending_moves_.end(),
                                 [piece_id](const PendingMove& move) {
                                     return move.piece_id == piece_id;
                                 });
    if (it != pending_moves_.end()) {
        pending_moves_.erase(it);
    }
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

void MoveScheduler::schedule_rest(Piece::Id piece_id, RestKind kind, int start_time_ms,
                                  int end_time_ms, std::size_t row, std::size_t col) {
    active_rests_[piece_id] = ActiveRest{kind, start_time_ms, end_time_ms, row, col};
}

void MoveScheduler::clear_rest(Piece::Id piece_id) {
    active_rests_.erase(piece_id);
}

bool MoveScheduler::is_piece_resting(uint64_t current_time_ms, Piece::Id piece_id) const {
    const auto it = active_rests_.find(piece_id);
    if (it == active_rests_.end()) {
        return false;
    }
    return static_cast<std::int64_t>(current_time_ms) < it->second.end_time_ms;
}

std::optional<RestKind> MoveScheduler::rest_kind(uint64_t current_time_ms,
                                                   Piece::Id piece_id) const {
    const auto it = active_rests_.find(piece_id);
    if (it == active_rests_.end()) {
        return std::nullopt;
    }
    if (static_cast<std::int64_t>(current_time_ms) >= it->second.end_time_ms) {
        return std::nullopt;
    }
    return it->second.kind;
}

AnimationSnapshot MoveScheduler::animations_at(std::int64_t clock_ms) const {
    // In-flight pieces are off the board grid; snapshots are the sole render-time source
    // for their position between start_pos and end_pos.
    AnimationSnapshot snapshot;

    for (const PendingMove& move : pending_moves_) {
        if (clock_ms >= move.arrival_time) {
            continue;
        }

        const auto [from_row, from_col] = move.start_pos;
        const auto [to_row, to_col] = move.end_pos;
        snapshot.moves.push_back({
            move.piece_id,
            move.kind,
            move.color,
            from_row,
            from_col,
            to_row,
            to_col,
            compute_animation_progress(clock_ms, move.start_time, move.arrival_time),
        });
    }

    for (const JumpState& jump : active_jumps_) {
        if (clock_ms >= jump.arrival_time) {
            continue;
        }

        const auto [row, col] = jump.cell;
        snapshot.jumps.push_back({
            jump.piece_id,
            jump.kind,
            jump.color,
            row,
            col,
            compute_animation_progress(clock_ms, jump.start_time, jump.arrival_time),
        });
    }

    for (const auto& [piece_id, rest] : active_rests_) {
        if (clock_ms >= rest.end_time_ms) {
            continue;
        }

        snapshot.rests.push_back({
            piece_id,
            rest.row,
            rest.col,
            rest.kind,
            compute_animation_progress(clock_ms, rest.start_time_ms, rest.end_time_ms),
        });
    }

    return snapshot;
}

void MoveScheduler::expire_jumps(
    uint64_t current_time_ms, const std::function<void(const JumpState&)>& on_complete) {
    const auto clock_ms = static_cast<std::int64_t>(current_time_ms);
    std::vector<JumpState> still_jumping;
    still_jumping.reserve(active_jumps_.size());

    for (JumpState& jump : active_jumps_) {
        if (clock_ms < jump.arrival_time) {
            still_jumping.push_back(std::move(jump));
        } else if (on_complete) {
            on_complete(jump);
        }
    }

    active_jumps_ = std::move(still_jumping);
}

void MoveScheduler::expire_rests(uint64_t current_time_ms) {
    const auto clock_ms = static_cast<std::int64_t>(current_time_ms);
    for (auto it = active_rests_.begin(); it != active_rests_.end();) {
        if (clock_ms >= it->second.end_time_ms) {
            it = active_rests_.erase(it);
        } else {
            ++it;
        }
    }
}

}  // namespace kfc
