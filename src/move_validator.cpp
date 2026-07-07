#include "move_validator.h"

#include <cstdlib>

namespace kfc {

namespace {

[[nodiscard]] bool is_in_bounds(const Board& board, int row, int col) noexcept {
    if (row < 0 || col < 0) {
        return false;
    }
    const auto urow = static_cast<std::size_t>(row);
    const auto ucol = static_cast<std::size_t>(col);
    return urow < board.size() && ucol < board[urow].size();
}

[[nodiscard]] bool is_path_clear(const Board& board, int start_row, int start_col, int end_row,
                                 int end_col) {
    const int dr = end_row - start_row;
    const int dc = end_col - start_col;
    const int step_r = (dr == 0) ? 0 : (dr > 0 ? 1 : -1);
    const int step_c = (dc == 0) ? 0 : (dc > 0 ? 1 : -1);

    int row = start_row + step_r;
    int col = start_col + step_c;
    while (row != end_row || col != end_col) {
        if (board[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] != ".") {
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

[[nodiscard]] bool is_rook_move(const Board& board, int start_row, int start_col, int end_row,
                                int end_col) {
    if (start_row != end_row && start_col != end_col) {
        return false;
    }
    return is_path_clear(board, start_row, start_col, end_row, end_col);
}

[[nodiscard]] bool is_bishop_move(const Board& board, int start_row, int start_col, int end_row,
                                  int end_col) {
    const int dr = end_row - start_row;
    const int dc = end_col - start_col;
    if (std::abs(dr) != std::abs(dc)) {
        return false;
    }
    return is_path_clear(board, start_row, start_col, end_row, end_col);
}

[[nodiscard]] bool is_queen_move(const Board& board, int start_row, int start_col, int end_row,
                                 int end_col) {
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

// Validates pawn movement and capture rules for the piece at (start_row, start_col).
// White pawns advance toward lower row indices (dr = -1); black pawns advance toward
// higher row indices (dr = +1). Only single-step forward moves and diagonal captures are
// allowed—no double moves, backward movement, or forward captures.
[[nodiscard]] bool is_pawn_move(const Board& board, int start_row, int start_col, int end_row,
                                int end_col) {
    const int dr = end_row - start_row;
    const int dc = end_col - start_col;

    const char pawn_color =
        board[static_cast<std::size_t>(start_row)][static_cast<std::size_t>(start_col)][0];
    const std::string& dest =
        board[static_cast<std::size_t>(end_row)][static_cast<std::size_t>(end_col)];

    // Straight forward: one row in the pawn's advance direction, same column, empty square.
    if (dc == 0) {
        if (pawn_color == 'w' && dr == -1) {
            return dest == ".";
        }
        if (pawn_color == 'b' && dr == 1) {
            return dest == ".";
        }
        return false;
    }

    // Diagonal capture: one row forward and one column sideways, enemy piece required.
    if (std::abs(dc) == 1) {
        if (dest == ".") {
            return false;
        }
        if (pawn_color == 'w' && dr == -1) {
            return dest[0] != 'w';
        }
        if (pawn_color == 'b' && dr == 1) {
            return dest[0] != 'b';
        }
    }

    return false;
}

[[nodiscard]] bool is_destination_legal(const Board& board, int start_row, int start_col,
                                        int end_row, int end_col) {
    const std::string& dest =
        board[static_cast<std::size_t>(end_row)][static_cast<std::size_t>(end_col)];
    if (dest == ".") {
        return true;
    }
    const char moving_color =
        board[static_cast<std::size_t>(start_row)][static_cast<std::size_t>(start_col)][0];
    return dest[0] != moving_color;
}

}  // namespace

bool is_legal_move(const Board& board, char piece, int start_row, int start_col, int end_row,
                   int end_col) {
    if (board.empty() || board.front().empty()) {
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

    // Route to the piece-specific helper; pawns handle destination rules internally.
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
