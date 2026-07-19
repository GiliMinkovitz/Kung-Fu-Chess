#include "real_time_arbiter.h"

#include "../model/board_model.h"
#include "../rules/game_rules.h"
#include "../rules/rules_registry.h"
#include "../rules/piece_rules/pawn_rules.h"

namespace kfc {

namespace {

// Re-validated at arrival because the board may have changed while the move was in flight
// (e.g. another piece moved onto the path or a friendly piece claimed the destination).
// The source cell is cleared when the move starts; validate the moving piece by id.
[[nodiscard]] bool can_settle_move(const BoardModel& board, const PendingMove& move) {
    const auto [start_row, start_col] = move.start_pos;
    const auto [end_row, end_col] = move.end_pos;

    const Piece& piece = board.get_piece(move.piece_id);

    const piece_rules::PieceRuleEntry rule = piece_rules::get_rule_entry(piece.kind);
    if (rule.validator == nullptr) {
        return false;
    }
    if (piece.kind == PieceKind::Pawn) {
        if (!piece_rules::is_pawn_move_for_piece(board, piece, static_cast<int>(start_row),
                                                 static_cast<int>(start_col),
                                                 static_cast<int>(end_row),
                                                 static_cast<int>(end_col))) {
            return false;
        }
    } else if (!rule.validator(board, static_cast<int>(start_row), static_cast<int>(start_col),
                               static_cast<int>(end_row), static_cast<int>(end_col))) {
        return false;
    }
    if (rule.requires_destination_check) {
        const Piece* dest = board.piece_at(end_row, end_col);
        if (dest != nullptr && !piece.is_opponent_of(*dest)) {
            return false;
        }
    }
    return true;
}

void restore_aborted_move(BoardModel& board, const PendingMove& move) {
    const auto [start_row, start_col] = move.start_pos;
    Piece& piece = board.get_piece(move.piece_id);
    piece.state = PieceState::Idle;

    if (const Piece* at_start = board.piece_at(start_row, start_col);
        at_start != nullptr && at_start->id == move.piece_id) {
        return;
    }

    if (!board.is_empty(start_row, start_col)) {
        board.remove_piece_at(start_row, start_col);
    }

    Piece updated = board.get_piece(move.piece_id);
    updated.cell = Position{static_cast<int>(start_row), static_cast<int>(start_col)};
    board.place_piece_at(start_row, start_col, std::move(updated));
}

}  // namespace

void RealTimeArbiter::update_time(std::int64_t ms, BoardModel& board, const GameRules& rules,
                                  bool& game_over) {
    clock_ms_ += ms;
    settle_pending_moves(board, rules, game_over);
}

void RealTimeArbiter::settle_pending_moves(BoardModel& board, const GameRules& rules,
                                           bool& game_over) {
    const uint64_t current_time_ms = static_cast<uint64_t>(clock_ms_);
    // MoveScheduler removes each due move from the queue before invoking the callback;
    // this callback owns all board mutations for that arrival.
    scheduler_.for_each_pending_due(current_time_ms, [this, &board, &rules, &game_over,
                                                      current_time_ms](const PendingMove& move) {
        if (!can_settle_move(board, move)) {
            restore_aborted_move(board, move);
            return;
        }

        const auto [start_row, start_col] = move.start_pos;
        const auto [end_row, end_col] = move.end_pos;

        if (const Piece* at_start = board.piece_at(start_row, start_col);
            at_start != nullptr && at_start->id == move.piece_id) {
            // Legacy path: source still occupied (direct arbiter calls without early release).
            board.clear_cell(start_row, start_col);
        }

        const ArrivingPieceInfo arriving_piece_info{
            move.piece_id,
            move.start_pos,
            move.end_pos,
        };

        const Piece& arriving_piece = board.get_piece(move.piece_id);
        const Piece* opponent_before = board.piece_at(end_row, end_col);
        const Piece::Id captured_opponent_id =
            opponent_before != nullptr && opponent_before->is_opponent_of(arriving_piece)
                ? opponent_before->id
                : Piece::kInvalidId;

        // CollisionResolver handles jump-capture and occupant capture; otherwise we place normally.
        const bool captured =
            scheduler_.check_for_jump_capture(current_time_ms, collision_resolver_, board, rules,
                                              move.end_pos, arriving_piece_info, game_over);
        Piece* destination_piece = board.piece_at(end_row, end_col);
        if (!captured) {
            Piece updated = board.get_piece(move.piece_id);
            updated = rules.on_reach_last_row(updated, end_row, board.rows());
            updated.cell = Position{static_cast<int>(end_row), static_cast<int>(end_col)};
            updated.state = PieceState::Idle;
            board.place_piece(std::move(updated));
            scheduler_.schedule_rest(
                move.piece_id, RestKind::Long, static_cast<int>(move.arrival_time),
                static_cast<int>(move.arrival_time + rules.long_rest_duration_ms), end_row,
                end_col);
        } else if (destination_piece != nullptr && destination_piece->id == move.piece_id) {
            if (captured_opponent_id != Piece::kInvalidId) {
                scheduler_.clear_rest(captured_opponent_id);
            }
            scheduler_.schedule_rest(
                move.piece_id, RestKind::Long, static_cast<int>(move.arrival_time),
                static_cast<int>(move.arrival_time + rules.long_rest_duration_ms), end_row,
                end_col);
        } else {
            scheduler_.clear_rest(move.piece_id);
            Piece& captured_arriving_piece = board.get_piece(move.piece_id);
            captured_arriving_piece.state = PieceState::Captured;
        }
    });

    scheduler_.expire_jumps(current_time_ms, [this, &rules](const JumpState& jump) {
        scheduler_.schedule_rest(jump.piece_id, RestKind::Short,
                                 static_cast<int>(jump.arrival_time),
                                 static_cast<int>(jump.arrival_time + rules.short_rest_duration_ms),
                                 jump.cell.first, jump.cell.second);
    });
    scheduler_.expire_rests(current_time_ms);
}

bool RealTimeArbiter::is_piece_moving(std::size_t row, std::size_t col) const {
    return scheduler_.is_piece_moving(static_cast<uint64_t>(clock_ms_), row, col);
}

bool RealTimeArbiter::is_piece_jumping(std::size_t row, std::size_t col) const {
    return scheduler_.is_piece_jumping(static_cast<uint64_t>(clock_ms_), row, col);
}

bool RealTimeArbiter::is_piece_resting(Piece::Id piece_id) const {
    return scheduler_.is_piece_resting(static_cast<uint64_t>(clock_ms_), piece_id);
}

bool RealTimeArbiter::is_same_color_destination_claimed(
    PieceColor color, const std::pair<std::size_t, std::size_t>& end_pos) const {
    return scheduler_.is_same_color_destination_claimed(static_cast<uint64_t>(clock_ms_), color,
                                                        end_pos);
}

bool RealTimeArbiter::would_conflict_with_opposite_color_move(
    PieceColor moving_color, Piece::Id piece_id,
    const std::pair<std::size_t, std::size_t>& start_pos,
    const std::pair<std::size_t, std::size_t>& end_pos, std::int64_t move_duration_ms) const {
    const PendingMove proposed{
        piece_id,
        moving_color,
        PieceKind::Pawn,
        start_pos,
        end_pos,
        clock_ms_,
        clock_ms_ + move_duration_ms,
    };
    return scheduler_.conflicts_with_opposite_color_move(static_cast<uint64_t>(clock_ms_),
                                                         moving_color, proposed);
}

void RealTimeArbiter::request_move(Piece::Id piece_id, PieceColor color, PieceKind kind,
                                   const std::pair<std::size_t, std::size_t>& start_pos,
                                   const std::pair<std::size_t, std::size_t>& end_pos,
                                   std::int64_t move_duration_ms) {
    scheduler_.schedule_move(PendingMove{
        piece_id,
        color,
        kind,
        start_pos,
        end_pos,
        clock_ms_,
        clock_ms_ + move_duration_ms,
    });
}

void RealTimeArbiter::request_jump(Piece::Id piece_id, PieceColor color,
                                   const std::pair<std::size_t, std::size_t>& cell,
                                   std::int64_t jump_duration_ms) {
    scheduler_.schedule_jump(JumpState{
        piece_id,
        color,
        cell,
        clock_ms_,
        clock_ms_ + jump_duration_ms,
    });
}

AnimationSnapshot RealTimeArbiter::animations_for_render() const {
    return scheduler_.animations_at(clock_ms_);
}

}  // namespace kfc
