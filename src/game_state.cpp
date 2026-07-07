#include "game_state.h"

#include <algorithm>

namespace kfc {

GameState::GameState(Board board) : board_(std::move(board)) {}

bool GameState::selection(std::size_t& row, std::size_t& col) const {
    if (!selected_) {
        return false;
    }
    row = selected_->first;
    col = selected_->second;
    return true;
}

bool GameState::is_in_bounds(std::size_t row, std::size_t col) const noexcept {
    if (board_.empty()) {
        return false;
    }
    return row < board_.size() && col < board_.front().size();
}

bool GameState::is_piece(std::size_t row, std::size_t col) const {
    if (!is_in_bounds(row, col)) {
        return false;
    }
    return board_[row][col] != ".";
}

bool GameState::has_pending_move_from(std::size_t row, std::size_t col) const {
    for (const PendingMove& move : pending_moves_) {
        if (clock_ms_ < move.arrival_time && move.start_pos.first == row &&
            move.start_pos.second == col) {
            return true;
        }
    }
    return false;
}

bool GameState::is_selectable_piece(std::size_t row, std::size_t col) const {
    return is_piece(row, col) && !has_pending_move_from(row, col);
}

bool GameState::is_friendly_to_selection(std::size_t row, std::size_t col) const {
    if (!selected_ || !is_selectable_piece(row, col)) {
        return false;
    }

    const std::string& selected_token = board_[selected_->first][selected_->second];
    return board_[row][col][0] == selected_token[0];
}

void GameState::add_clock(std::int64_t ms) {
    clock_ms_ += ms;
    settle_pending_moves();
}

void GameState::settle_pending_moves() {
    std::vector<PendingMove> still_pending;
    still_pending.reserve(pending_moves_.size());

    for (PendingMove& move : pending_moves_) {
        if (clock_ms_ < move.arrival_time) {
            still_pending.push_back(std::move(move));
            continue;
        }

        const auto [start_row, start_col] = move.start_pos;
        const auto [end_row, end_col] = move.end_pos;
        board_[start_row][start_col] = ".";
        board_[end_row][end_col] = move.piece;
    }

    pending_moves_ = std::move(still_pending);
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
    if (has_pending_move_from(from_row, from_col)) {
        return;
    }

    const std::size_t row_delta =
        from_row > to_row ? from_row - to_row : to_row - from_row;
    const std::size_t col_delta =
        from_col > to_col ? from_col - to_col : to_col - from_col;
    const std::int64_t move_duration =
        static_cast<std::int64_t>(std::max(row_delta, col_delta)) * kMoveDurationMs;

    pending_moves_.push_back(PendingMove{
        board_[from_row][from_col],
        {from_row, from_col},
        {to_row, to_col},
        clock_ms_ + move_duration,
    });
    clear_selection();
}

}  // namespace kfc
