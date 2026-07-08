#include "game_state.h"

#include <algorithm>

namespace kfc {

namespace {

[[nodiscard]] bool ranges_overlap(std::size_t a_min, std::size_t a_max, std::size_t b_min,
                                  std::size_t b_max) {
    return a_min <= b_max && b_min <= a_max;
}

[[nodiscard]] bool path_cells_intersect(const std::pair<std::size_t, std::size_t>& a_start,
                                        const std::pair<std::size_t, std::size_t>& a_end,
                                        const std::pair<std::size_t, std::size_t>& b_start,
                                        const std::pair<std::size_t, std::size_t>& b_end) {
    const int a_dr = static_cast<int>(a_end.first) - static_cast<int>(a_start.first);
    const int a_dc = static_cast<int>(a_end.second) - static_cast<int>(a_start.second);
    const int b_dr = static_cast<int>(b_end.first) - static_cast<int>(b_start.first);
    const int b_dc = static_cast<int>(b_end.second) - static_cast<int>(b_start.second);

    if ((a_dr != 0 && a_dc != 0 && std::abs(a_dr) != std::abs(a_dc)) ||
        (b_dr != 0 && b_dc != 0 && std::abs(b_dr) != std::abs(b_dc))) {
        return a_start == b_start || a_start == b_end || a_end == b_start || a_end == b_end;
    }

    const int a_step_r = (a_dr == 0) ? 0 : (a_dr > 0 ? 1 : -1);
    const int a_step_c = (a_dc == 0) ? 0 : (a_dc > 0 ? 1 : -1);
    const int b_step_r = (b_dr == 0) ? 0 : (b_dr > 0 ? 1 : -1);
    const int b_step_c = (b_dc == 0) ? 0 : (b_dc > 0 ? 1 : -1);

    int a_row = static_cast<int>(a_start.first);
    int a_col = static_cast<int>(a_start.second);
    while (true) {
        int b_row = static_cast<int>(b_start.first);
        int b_col = static_cast<int>(b_start.second);
        while (true) {
            if (a_row == b_row && a_col == b_col) {
                return true;
            }
            if (b_row == static_cast<int>(b_end.first) &&
                b_col == static_cast<int>(b_end.second)) {
                break;
            }
            b_row += b_step_r;
            b_col += b_step_c;
        }

        if (a_row == static_cast<int>(a_end.first) && a_col == static_cast<int>(a_end.second)) {
            break;
        }
        a_row += a_step_r;
        a_col += a_step_c;
    }

    return false;
}

[[nodiscard]] bool has_common_route(const PendingMove& a, const PendingMove& b) {
    if (path_cells_intersect(a.start_pos, a.end_pos, b.start_pos, b.end_pos)) {
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

[[nodiscard]] bool conflicts_with_opposite_color_move(
    const std::vector<PendingMove>& pending_moves, std::int64_t clock_ms, char moving_color,
    const PendingMove& proposed) {
    for (const PendingMove& existing : pending_moves) {
        if (clock_ms >= existing.arrival_time) {
            continue;
        }
        if (existing.piece.color == moving_color) {
            continue;
        }
        if (has_common_route(existing, proposed)) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool is_same_color_destination_claimed(
    const std::vector<PendingMove>& pending_moves, std::int64_t clock_ms, char moving_color,
    const std::pair<std::size_t, std::size_t>& end_pos) {
    for (const PendingMove& existing : pending_moves) {
        if (clock_ms >= existing.arrival_time) {
            continue;
        }
        if (existing.piece.color != moving_color) {
            continue;
        }
        if (existing.end_pos == end_pos) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool can_settle_move(const BoardModel& board, const GameRules& rules,
                                   const PendingMove& move) {
    const auto [start_row, start_col] = move.start_pos;
    const auto [end_row, end_col] = move.end_pos;

    if (board.piece_at(start_row, start_col) != move.piece) {
        return false;
    }

    return rules.is_legal_move(board, move.piece.type, static_cast<int>(start_row),
                               static_cast<int>(start_col), static_cast<int>(end_row),
                               static_cast<int>(end_col));
}

[[nodiscard]] Piece apply_on_reach_last_row(const GameRules& rules, Piece piece,
                                            std::size_t end_row, std::size_t board_rows) {
    return rules.on_reach_last_row(piece, end_row, board_rows);
}

}  // namespace

GameState::GameState(BoardModel board) : GameState(std::move(board), KungFuChessRules::standard()) {}

GameState::GameState(BoardModel board, GameRules rules)
    : board_(std::move(board)), rules_(std::move(rules)) {}

Piece GameState::piece_at(std::size_t row, std::size_t col) const {
    return board_.piece_at(row, col);
}

bool GameState::is_empty(std::size_t row, std::size_t col) const {
    return board_.is_empty(row, col);
}

void GameState::set_piece(std::size_t row, std::size_t col, Piece piece) {
    board_.set_piece(row, col, piece);
}

bool GameState::selection(std::size_t& row, std::size_t& col) const {
    if (!selected_) {
        return false;
    }
    row = selected_->first;
    col = selected_->second;
    return true;
}

bool GameState::is_in_bounds(std::size_t row, std::size_t col) const noexcept {
    return board_.is_in_bounds(row, col);
}

bool GameState::is_piece(std::size_t row, std::size_t col) const {
    if (!is_in_bounds(row, col)) {
        return false;
    }
    return !board_.is_empty(row, col);
}

bool GameState::is_piece_moving(std::size_t row, std::size_t col) const {
    for (const PendingMove& move : pending_moves_) {
        if (clock_ms_ < move.arrival_time && move.start_pos.first == row &&
            move.start_pos.second == col) {
            return true;
        }
    }
    return false;
}

bool GameState::is_piece_jumping(std::size_t row, std::size_t col) const {
    for (const JumpState& jump : active_jumps_) {
        if (clock_ms_ < jump.arrival_time && jump.cell.first == row &&
            jump.cell.second == col) {
            return true;
        }
    }
    return false;
}

bool GameState::is_selectable_piece(std::size_t row, std::size_t col) const {
    return is_piece(row, col) && !is_piece_moving(row, col) && !is_piece_jumping(row, col);
}

bool GameState::is_friendly_to_selection(std::size_t row, std::size_t col) const {
    if (!selected_ || !is_selectable_piece(row, col)) {
        return false;
    }

    const Piece selected_piece = board_.piece_at(selected_->first, selected_->second);
    return board_.piece_at(row, col).color == selected_piece.color;
}

bool GameState::is_square_claimed_by_same_color_pending_move(std::size_t row, std::size_t col,
                                                             char color) const {
    return is_same_color_destination_claimed(pending_moves_, clock_ms_, color, {row, col});
}

bool GameState::is_legal_move(int start_row, int start_col, int end_row, int end_col) const {
    const Piece moving =
        board_.piece_at(static_cast<std::size_t>(start_row), static_cast<std::size_t>(start_col));
    return rules_.is_legal_move(board_, moving.type, start_row, start_col, end_row, end_col);
}

void GameState::add_clock(std::int64_t ms) {
    clock_ms_ += ms;
    settle_pending_moves();
}

void GameState::expire_jumps() {
    std::vector<JumpState> still_jumping;
    still_jumping.reserve(active_jumps_.size());

    for (JumpState& jump : active_jumps_) {
        if (clock_ms_ < jump.arrival_time) {
            still_jumping.push_back(std::move(jump));
        }
    }

    active_jumps_ = std::move(still_jumping);
}

bool GameState::check_for_jump_capture(
    const std::pair<std::size_t, std::size_t>& target_cell,
    const ArrivingPieceInfo& arriving_piece_info) {
    const auto [end_row, end_col] = target_cell;

    for (const JumpState& jump : active_jumps_) {
        if (clock_ms_ <= jump.arrival_time && jump.cell == target_cell &&
            arriving_piece_info.piece.color != jump.piece.color) {
            if (rules_.is_game_over(arriving_piece_info.piece)) {
                game_over_ = true;
            }
            return true;
        }
    }

    const Piece destination = board_.piece_at(end_row, end_col);
    if (!destination.is_empty() && destination.color != arriving_piece_info.piece.color) {
        if (rules_.is_game_over(destination)) {
            game_over_ = true;
        }
        board_.set_piece(end_row, end_col,
                         apply_on_reach_last_row(rules_, arriving_piece_info.piece, end_row,
                                                 board_.rows()));
        return true;
    }

    return false;
}

void GameState::settle_pending_moves() {
    std::vector<PendingMove> still_pending;
    still_pending.reserve(pending_moves_.size());

    for (PendingMove& move : pending_moves_) {
        if (clock_ms_ < move.arrival_time) {
            still_pending.push_back(std::move(move));
            continue;
        }

        if (!can_settle_move(board_, rules_, move)) {
            continue;
        }

        const auto [start_row, start_col] = move.start_pos;
        const auto [end_row, end_col] = move.end_pos;

        board_.set_piece(start_row, start_col, Piece::empty());

        const ArrivingPieceInfo arriving_piece_info{
            move.piece,
            move.start_pos,
            move.end_pos,
        };

        if (!check_for_jump_capture(move.end_pos, arriving_piece_info)) {
            board_.set_piece(end_row, end_col,
                             apply_on_reach_last_row(rules_, move.piece, end_row, board_.rows()));
        }
    }

    pending_moves_ = std::move(still_pending);
    expire_jumps();
}

void GameState::select(std::size_t row, std::size_t col) {
    if (!is_in_bounds(row, col)) {
        return;
    }
    selected_ = std::make_pair(row, col);
}

void GameState::clear_selection() {
    selected_.reset();
}

void GameState::move_selected_to(std::size_t to_row, std::size_t to_col) {
    if (!selected_ || !is_in_bounds(to_row, to_col)) {
        return;
    }

    const auto [from_row, from_col] = *selected_;
    if (is_piece_moving(from_row, from_col) || is_piece_jumping(from_row, from_col)) {
        return;
    }

    const std::size_t row_delta =
        from_row > to_row ? from_row - to_row : to_row - from_row;
    const std::size_t col_delta =
        from_col > to_col ? from_col - to_col : to_col - from_col;

    const Piece moving = board_.piece_at(from_row, from_col);
    const Piece destination = board_.piece_at(to_row, to_col);
    const bool is_capture =
        !destination.is_empty() && destination.color != moving.color;
    const std::int64_t move_duration =
        is_capture ? rules_.move_duration_ms
                   : static_cast<std::int64_t>(std::max(row_delta, col_delta)) *
                         rules_.move_duration_ms;

    if (!rules_.is_legal_move(board_, moving.type, static_cast<int>(from_row),
                              static_cast<int>(from_col), static_cast<int>(to_row),
                              static_cast<int>(to_col))) {
        return;
    }

    if (is_same_color_destination_claimed(pending_moves_, clock_ms_, moving.color,
                                          {to_row, to_col})) {
        return;
    }

    const PendingMove proposed{
        moving,
        {from_row, from_col},
        {to_row, to_col},
        clock_ms_ + move_duration,
    };
    if (conflicts_with_opposite_color_move(pending_moves_, clock_ms_, moving.color, proposed)) {
        return;
    }

    pending_moves_.push_back(proposed);
    clear_selection();
}

void GameState::jump_selected() {
    if (!selected_) {
        return;
    }

    const auto [row, col] = *selected_;
    jump_at(row, col);
    clear_selection();
}

void GameState::jump_at(std::size_t row, std::size_t col) {
    if (!is_in_bounds(row, col) || board_.is_empty(row, col)) {
        return;
    }

    if (is_piece_moving(row, col) || is_piece_jumping(row, col)) {
        return;
    }

    active_jumps_.push_back(JumpState{
        board_.piece_at(row, col),
        {row, col},
        clock_ms_ + rules_.jump_duration_ms,
    });
}

}  // namespace kfc
