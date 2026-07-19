#include "pawn_rules.h"

#include "../../model/piece.h"

#include <cstdlib>

namespace kfc::piece_rules {

namespace {

[[nodiscard]] bool is_pawn_start_row(const BoardModel& board, PieceColor pawn_color,
                                     int start_row) noexcept {
    const int rows = static_cast<int>(board.rows());
    if (rows < 4) {
        return false;
    }
    if (pawn_color == PieceColor::White) {
        return start_row == rows - 2;
    }
    return start_row == rows - 4;
}

}  // namespace

bool is_pawn_move_for_piece(const BoardModel& board, const Piece& moving, int start_row,
                            int start_col, int end_row, int end_col) {
    const int dr = end_row - start_row;
    const int dc = end_col - start_col;

    const Piece* dest =
        board.piece_at(static_cast<std::size_t>(end_row), static_cast<std::size_t>(end_col));

    if (dc == 0) {
        if (dest != nullptr) {
            return false;
        }

        if (moving.is_white() && dr == -1) {
            return true;
        }
        if (moving.is_black() && dr == 1) {
            return true;
        }

        if (moving.is_white() && dr == -2 &&
            is_pawn_start_row(board, moving.color, start_row)) {
            const int mid_row = start_row - 1;
            return board.is_empty(static_cast<std::size_t>(mid_row),
                                  static_cast<std::size_t>(start_col));
        }
        if (moving.is_black() && dr == 2 && is_pawn_start_row(board, moving.color, start_row)) {
            const int mid_row = start_row + 1;
            return board.is_empty(static_cast<std::size_t>(mid_row),
                                  static_cast<std::size_t>(start_col));
        }

        return false;
    }

    if (std::abs(dc) == 1) {
        if (dest == nullptr) {
            return false;
        }
        if (moving.is_white() && dr == -1) {
            return dest->is_black();
        }
        if (moving.is_black() && dr == 1) {
            return dest->is_white();
        }
    }

    return false;
}

bool is_pawn_move(const BoardModel& board, int start_row, int start_col, int end_row,
                  int end_col) {
    const Piece* moving = board.piece_at(static_cast<std::size_t>(start_row),
                                         static_cast<std::size_t>(start_col));
    if (moving == nullptr) {
        return false;
    }
    return is_pawn_move_for_piece(board, *moving, start_row, start_col, end_row, end_col);
}

}  // namespace kfc::piece_rules
