#include "real_time_arbiter.h"

#include "../model/board_model.h"
#include "../rules/game_rules.h"

namespace kfc {

namespace {

[[nodiscard]] bool can_settle_move(const BoardModel& board, const GameRules& rules,
                                   const PendingMove& move) {
    const auto [start_row, start_col] = move.start_pos;
    const auto [end_row, end_col] = move.end_pos;

    const Piece* piece = board.piece_at(start_row, start_col);
    if (piece == nullptr || piece->id != move.piece_id) {
        return false;
    }

    return rules.is_legal_move(board, piece->kind, static_cast<int>(start_row),
                               static_cast<int>(start_col), static_cast<int>(end_row),
                               static_cast<int>(end_col));
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
    scheduler_.for_each_pending_due(current_time_ms, [this, &board, &rules, &game_over,
                                                      current_time_ms](const PendingMove& move) {
        if (!can_settle_move(board, rules, move)) {
            const auto [start_row, start_col] = move.start_pos;
            if (Piece* piece = board.piece_at(start_row, start_col);
                piece != nullptr && piece->id == move.piece_id) {
                piece->state = PieceState::Idle;
            }
            return;
        }

        const auto [start_row, start_col] = move.start_pos;
        const auto [end_row, end_col] = move.end_pos;

        board.clear_cell(start_row, start_col);

        const ArrivingPieceInfo arriving_piece_info{
            move.piece_id,
            move.start_pos,
            move.end_pos,
        };

        if (!scheduler_.check_for_jump_capture(current_time_ms, collision_resolver_, board, rules,
                                               move.end_pos, arriving_piece_info, game_over)) {
            Piece updated = board.get_piece(move.piece_id);
            updated = rules.on_reach_last_row(updated, end_row, board.rows());
            updated.cell = Position{static_cast<int>(end_row), static_cast<int>(end_col)};
            updated.state = PieceState::Idle;
            board.place_piece(std::move(updated));
        } else {
            Piece& arriving_piece = board.get_piece(move.piece_id);
            arriving_piece.state = PieceState::Captured;
        }
    });

    scheduler_.expire_jumps(current_time_ms);
}

bool RealTimeArbiter::is_piece_moving(std::size_t row, std::size_t col) const {
    return scheduler_.is_piece_moving(static_cast<uint64_t>(clock_ms_), row, col);
}

bool RealTimeArbiter::is_piece_jumping(std::size_t row, std::size_t col) const {
    return scheduler_.is_piece_jumping(static_cast<uint64_t>(clock_ms_), row, col);
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
        start_pos,
        end_pos,
        clock_ms_,
        clock_ms_ + move_duration_ms,
    };
    return scheduler_.conflicts_with_opposite_color_move(static_cast<uint64_t>(clock_ms_),
                                                         moving_color, proposed);
}

void RealTimeArbiter::request_move(Piece::Id piece_id, PieceColor color,
                                   const std::pair<std::size_t, std::size_t>& start_pos,
                                   const std::pair<std::size_t, std::size_t>& end_pos,
                                   std::int64_t move_duration_ms) {
    scheduler_.schedule_move(PendingMove{
        piece_id,
        color,
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
