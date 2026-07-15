#include "ui/view/board_view_model.h"

namespace kfc {

namespace {

CellView kEmptyCellView{};

}  // namespace

std::size_t board_view_cell_index(std::size_t row, std::size_t col, std::size_t width) noexcept {
    return row * width + col;
}

const CellView& board_view_cell_at(const BoardViewModel& view, std::size_t row, std::size_t col) {
    if (row >= view.height || col >= view.width) {
        return kEmptyCellView;
    }

    const std::size_t index = board_view_cell_index(row, col, view.width);
    if (index >= view.cells.size()) {
        return kEmptyCellView;
    }

    return view.cells[index];
}

std::optional<PieceView> board_view_piece_at(const BoardViewModel& view, std::size_t row,
                                             std::size_t col) {
    return board_view_cell_at(view, row, col).piece;
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
