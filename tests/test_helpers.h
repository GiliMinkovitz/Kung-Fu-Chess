#pragma once

#include "core/board_model.h"
#include "logic/game_state.h"

#include <string>
#include <vector>

namespace kfc::test {

inline BoardModel make_board(std::initializer_list<std::initializer_list<const char*>> rows) {
    return BoardModel::from_token_grid(rows);
}

using BoardLayout = std::vector<std::vector<std::string>>;

inline BoardLayout capture_layout(const GameState& state) {
    BoardLayout layout(state.rows(), std::vector<std::string>(state.cols()));
    for (std::size_t row = 0; row < state.rows(); ++row) {
        for (std::size_t col = 0; col < state.cols(); ++col) {
            layout[row][col] = state.token_at(row, col);
        }
    }
    return layout;
}

inline bool layout_matches(const GameState& state, const BoardLayout& layout) {
    if (state.rows() != layout.size()) {
        return false;
    }
    for (std::size_t row = 0; row < state.rows(); ++row) {
        if (state.cols() != layout[row].size()) {
            return false;
        }
        for (std::size_t col = 0; col < state.cols(); ++col) {
            if (state.token_at(row, col) != layout[row][col]) {
                return false;
            }
        }
    }
    return true;
}

}  // namespace kfc::test
