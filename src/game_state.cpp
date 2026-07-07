#include "game_state.h"

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

bool GameState::is_friendly_to_selection(std::size_t row, std::size_t col) const {
    if (!selected_ || !is_piece(row, col)) {
        return false;
    }

    const std::string& selected_token = board_[selected_->first][selected_->second];
    return board_[row][col][0] == selected_token[0];
}

void GameState::add_clock(std::int64_t ms) {
    clock_ms_ += ms;
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
    const std::string piece = board_[from_row][from_col];
    board_[from_row][from_col] = ".";
    board_[to_row][to_col] = piece;
    clear_selection();
}

}  // namespace kfc
