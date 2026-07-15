#include "board_view_model.h"

namespace kfc {

std::size_t board_view_cell_index(std::size_t row, std::size_t col, std::size_t cols) noexcept {
    return row * cols + col;
}

std::string board_view_token_at(const BoardViewModel& view, std::size_t row, std::size_t col) {
    if (row >= view.rows || col >= view.cols) {
        return std::string(1, kEmptyToken);
    }

    const std::size_t index = board_view_cell_index(row, col, view.cols);
    if (index >= view.tokens.size()) {
        return std::string(1, kEmptyToken);
    }

    return view.tokens[index];
}

bool board_view_is_move_origin(const BoardViewModel& view, std::size_t row, std::size_t col) {
    for (const ActiveMoveSnapshot& move : view.animations.moves) {
        if (move.from_row == row && move.from_col == col) {
            return true;
        }
    }
    return false;
}

bool board_view_is_jumping_cell(const BoardViewModel& view, std::size_t row, std::size_t col) {
    for (const ActiveJumpSnapshot& jump : view.animations.jumps) {
        if (jump.row == row && jump.col == col) {
            return true;
        }
    }
    return false;
}

float board_view_jump_progress_at(const BoardViewModel& view, std::size_t row, std::size_t col) {
    for (const ActiveJumpSnapshot& jump : view.animations.jumps) {
        if (jump.row == row && jump.col == col) {
            return jump.progress;
        }
    }
    return 0.0f;
}

}  // namespace kfc
