#include "collision_resolver.h"

#include "path_utils.h"

#include <algorithm>

namespace kfc {

namespace {

[[nodiscard]] bool ranges_overlap(std::size_t a_min, std::size_t a_max, std::size_t b_min,
                                  std::size_t b_max) {
    return a_min <= b_max && b_min <= a_max;
}

}  // namespace

bool CollisionResolver::has_common_route(const PendingMove& a, const PendingMove& b) {
    if (paths_share_cell(a.start_pos, a.end_pos, b.start_pos, b.end_pos)) {
        return true;
    }

    const bool a_horizontal = a.start_pos.first == a.end_pos.first;
    const bool b_horizontal = b.start_pos.first == b.end_pos.first;
    if (a_horizontal && b_horizontal) {
        const std::size_t a_min_col = std::min(a.start_pos.second, a.end_pos.second);
        const std::size_t a_max_col = std::max(a.start_pos.second, a.end_pos.second);
        const std::size_t b_min_col = std::min(b.start_pos.second, b.end_pos.second);
        const std::size_t b_max_col = std::max(b.start_pos.second, b.end_pos.second);
        return ranges_overlap(a_min_col, a_max_col, b_min_col, b_max_col);
    }

    const bool a_vertical = a.start_pos.second == a.end_pos.second;
    const bool b_vertical = b.start_pos.second == b.end_pos.second;
    if (a_vertical && b_vertical) {
        const std::size_t a_min_row = std::min(a.start_pos.first, a.end_pos.first);
        const std::size_t a_max_row = std::max(a.start_pos.first, a.end_pos.first);
        const std::size_t b_min_row = std::min(b.start_pos.first, b.end_pos.first);
        const std::size_t b_max_row = std::max(b.start_pos.first, b.end_pos.first);
        return ranges_overlap(a_min_row, a_max_row, b_min_row, b_max_row);
    }

    return false;
}

bool CollisionResolver::conflicts_with_opposite_color_move(
    const std::vector<PendingMove>& pending_moves, std::int64_t clock_ms,
    PieceColor moving_color, const PendingMove& proposed) {
    for (const PendingMove& existing : pending_moves) {
        if (clock_ms >= existing.arrival_time) {
            continue;
        }
        if (existing.color == moving_color) {
            continue;
        }
        if (has_common_route(existing, proposed)) {
            return true;
        }
    }
    return false;
}

bool CollisionResolver::is_same_color_destination_claimed(
    const std::vector<PendingMove>& pending_moves, std::int64_t clock_ms, PieceColor moving_color,
    const std::pair<std::size_t, std::size_t>& end_pos) {
    for (const PendingMove& existing : pending_moves) {
        if (clock_ms >= existing.arrival_time) {
            continue;
        }
        if (existing.color != moving_color) {
            continue;
        }
        if (existing.end_pos == end_pos) {
            return true;
        }
    }
    return false;
}

bool CollisionResolver::check_for_jump_capture(
    BoardModel& board, const GameRules& rules, std::int64_t clock_ms,
    const std::vector<JumpState>& active_jumps,
    const std::pair<std::size_t, std::size_t>& target_cell,
    const ArrivingPieceInfo& arriving_piece_info, bool& game_over) const {
    const auto [end_row, end_col] = target_cell;
    const Piece& arriving_piece = board.get_piece(arriving_piece_info.piece_id);

    for (const JumpState& jump : active_jumps) {
        if (clock_ms <= jump.arrival_time && jump.cell == target_cell &&
            arriving_piece.color != jump.color) {
            if (rules.is_game_over(arriving_piece)) {
                game_over = true;
            }
            return true;
        }
    }

    const Piece* destination = board.piece_at(end_row, end_col);
    if (destination != nullptr && destination->color != arriving_piece.color) {
        if (rules.is_game_over(*destination)) {
            game_over = true;
        }
        board.remove_piece_at(end_row, end_col);
        Piece placed = rules.on_reach_last_row(arriving_piece, end_row, board.rows());
        placed.cell = Position{static_cast<int>(end_row), static_cast<int>(end_col)};
        placed.state = PieceState::Idle;
        board.place_piece(std::move(placed));
        return true;
    }

    return false;
}

}  // namespace kfc
