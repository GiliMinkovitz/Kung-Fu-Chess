#include "move_validator.h"

#include <cstdlib>

namespace kfc {

namespace {

[[nodiscard]] bool is_in_bounds(const BoardModel& board, int row, int col) noexcept {
    if (row < 0 || col < 0) {
        return false;
    }
    const auto urow = static_cast<std::size_t>(row);
    const auto ucol = static_cast<std::size_t>(col);
    return board.is_in_bounds(urow, ucol);
}

[[nodiscard]] bool is_path_clear(const BoardModel& board, int start_row, int start_col,
                                 int end_row, int end_col) {
    const int dr = end_row - start_row;
    const int dc = end_col - start_col;
    const int step_r = (dr == 0) ? 0 : (dr > 0 ? 1 : -1);
    const int step_c = (dc == 0) ? 0 : (dc > 0 ? 1 : -1);

    int row = start_row + step_r;
    int col = start_col + step_c;
    while (row != end_row || col != end_col) {
        if (!board.is_empty(static_cast<std::size_t>(row), static_cast<std::size_t>(col))) {
            return false;
        }
        row += step_r;
        col += step_c;
    }
    return true;
}

[[nodiscard]] bool is_king_move(int dr, int dc) noexcept {
    const int adr = std::abs(dr);
    const int adc = std::abs(dc);
    return adr <= 1 && adc <= 1 && (adr != 0 || adc != 0);
}

[[nodiscard]] bool is_rook_move(const BoardModel& board, int start_row, int start_col,
                                int end_row, int end_col) {
    if (start_row != end_row && start_col != end_col) {
        return false;
    }
    return is_path_clear(board, start_row, start_col, end_row, end_col);
}

[[nodiscard]] bool is_bishop_move(const BoardModel& board, int start_row, int start_col,
                                  int end_row, int end_col) {
    const int dr = end_row - start_row;
    const int dc = end_col - start_col;
    if (std::abs(dr) != std::abs(dc)) {
        return false;
    }
    return is_path_clear(board, start_row, start_col, end_row, end_col);
}

[[nodiscard]] bool is_queen_move(const BoardModel& board, int start_row, int start_col,
                                 int end_row, int end_col) {
    const int dr = end_row - start_row;
    const int dc = end_col - start_col;
    const bool straight = start_row == end_row || start_col == end_col;
    const bool diagonal = std::abs(dr) == std::abs(dc);
    if (!straight && !diagonal) {
        return false;
    }
    return is_path_clear(board, start_row, start_col, end_row, end_col);
}

[[nodiscard]] bool is_knight_move(int dr, int dc) noexcept {
    const int adr = std::abs(dr);
    const int adc = std::abs(dc);
    return (adr == 2 && adc == 1) || (adr == 1 && adc == 2);
}

[[nodiscard]] bool is_pawn_start_row(const BoardModel& board, char pawn_color,
                                     int start_row) noexcept {
    if (board.rows() == 0) {
        return false;
    }
    if (pawn_color == 'w') {
        return start_row == static_cast<int>(board.rows()) - 1;
    }
    return start_row == 0;
}

[[nodiscard]] bool is_pawn_move(const BoardModel& board, int start_row, int start_col, int end_row,
                                int end_col) {
    const int dr = end_row - start_row;
    const int dc = end_col - start_col;

    const char pawn_color =
        board.piece_at(static_cast<std::size_t>(start_row), static_cast<std::size_t>(start_col))
            .color;
    const Piece dest =
        board.piece_at(static_cast<std::size_t>(end_row), static_cast<std::size_t>(end_col));

    if (dc == 0) {
        if (!dest.is_empty()) {
            return false;
        }

        if (pawn_color == 'w' && dr == -1) {
            return true;
        }
        if (pawn_color == 'b' && dr == 1) {
            return true;
        }

        if (pawn_color == 'w' && dr == -2 && is_pawn_start_row(board, pawn_color, start_row)) {
            const int mid_row = start_row - 1;
            return board.is_empty(static_cast<std::size_t>(mid_row),
                                  static_cast<std::size_t>(start_col));
        }
        if (pawn_color == 'b' && dr == 2 && is_pawn_start_row(board, pawn_color, start_row)) {
            const int mid_row = start_row + 1;
            return board.is_empty(static_cast<std::size_t>(mid_row),
                                  static_cast<std::size_t>(start_col));
        }

        return false;
    }

    if (std::abs(dc) == 1) {
        if (dest.is_empty()) {
            return false;
        }
        if (pawn_color == 'w' && dr == -1) {
            return dest.color != 'w';
        }
        if (pawn_color == 'b' && dr == 1) {
            return dest.color != 'b';
        }
    }

    return false;
}

[[nodiscard]] bool is_destination_legal(const BoardModel& board, int start_row, int start_col,
                                        int end_row, int end_col) {
    const Piece dest =
        board.piece_at(static_cast<std::size_t>(end_row), static_cast<std::size_t>(end_col));
    if (dest.is_empty()) {
        return true;
    }
    const char moving_color =
        board.piece_at(static_cast<std::size_t>(start_row), static_cast<std::size_t>(start_col))
            .color;
    return dest.color != moving_color;
}

}  // namespace

bool is_legal_move(const BoardModel& board, char piece, int start_row, int start_col, int end_row,
                   int end_col) {
    if (board.rows() == 0 || board.cols() == 0) {
        return false;
    }
    if (!is_in_bounds(board, start_row, start_col) || !is_in_bounds(board, end_row, end_col)) {
        return false;
    }
    if (start_row == end_row && start_col == end_col) {
        return false;
    }

    const int dr = end_row - start_row;
    const int dc = end_col - start_col;

    switch (piece) {
        case 'P':
            return is_pawn_move(board, start_row, start_col, end_row, end_col);
        case 'K':
            if (!is_king_move(dr, dc)) {
                return false;
            }
            break;
        case 'R':
            if (!is_rook_move(board, start_row, start_col, end_row, end_col)) {
                return false;
            }
            break;
        case 'B':
            if (!is_bishop_move(board, start_row, start_col, end_row, end_col)) {
                return false;
            }
            break;
        case 'Q':
            if (!is_queen_move(board, start_row, start_col, end_row, end_col)) {
                return false;
            }
            break;
        case 'N':
            if (!is_knight_move(dr, dc)) {
                return false;
            }
            break;
        default:
            return false;
    }

    return is_destination_legal(board, start_row, start_col, end_row, end_col);
}

}  // namespace kfc
