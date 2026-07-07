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

    switch (piece) {
        case 'K':
            return is_king_move(dr, dc);
        case 'R':
            return is_rook_move(board, start_row, start_col, end_row, end_col);
        case 'B':
            return is_bishop_move(board, start_row, start_col, end_row, end_col);
        case 'Q':
            return is_queen_move(board, start_row, start_col, end_row, end_col);
        case 'N':
            return is_knight_move(dr, dc);
        default:
            return false;
    }
}

}  // namespace kfc
