#include "move_validator.h"

#include "game_config.h"
#include "path_utils.h"

#include <cstdlib>

namespace kfc {

namespace {

[[nodiscard]] bool is_path_clear(const BoardModel& board, int start_row, int start_col,
                                 int end_row, int end_col) {
    bool clear = true;
    for_each_cell_on_path(start_row, start_col, end_row, end_col,
                          [&](int row, int col) {
                              if (!board.is_empty(static_cast<std::size_t>(row),
                                                  static_cast<std::size_t>(col))) {
                                  clear = false;
                              }
                          });
    return clear;
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
    if (pawn_color == kWhiteColor) {
        return start_row == static_cast<int>(board.rows()) - 1;
    }
    return start_row == 0;
}

[[nodiscard]] bool is_pawn_move(const BoardModel& board, int start_row, int start_col, int end_row,
                                int end_col) {
    const int dr = end_row - start_row;
    const int dc = end_col - start_col;

    const Piece moving =
        board.piece_at(static_cast<std::size_t>(start_row), static_cast<std::size_t>(start_col));
    const Piece dest =
        board.piece_at(static_cast<std::size_t>(end_row), static_cast<std::size_t>(end_col));

    if (dc == 0) {
        if (!dest.is_empty()) {
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
        if (dest.is_empty()) {
            return false;
        }
        if (moving.is_white() && dr == -1) {
            return dest.is_black();
        }
        if (moving.is_black() && dr == 1) {
            return dest.is_white();
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
    const Piece moving =
        board.piece_at(static_cast<std::size_t>(start_row), static_cast<std::size_t>(start_col));
    return moving.is_opponent_of(dest);
}

}  // namespace

bool is_legal_move(const BoardModel& board, char piece, int start_row, int start_col, int end_row,
                   int end_col) {
    if (board.rows() == 0 || board.cols() == 0) {
        return false;
    }
    if (!board.contains(start_row, start_col) || !board.contains(end_row, end_col)) {
        return false;
    }
    if (start_row == end_row && start_col == end_col) {
        return false;
    }

    const int dr = end_row - start_row;
    const int dc = end_col - start_col;

    switch (piece) {
        case kPawnType:
            return is_pawn_move(board, start_row, start_col, end_row, end_col);
        case kKingType:
            if (!is_king_move(dr, dc)) {
                return false;
            }
            break;
        case kRookType:
            if (!is_rook_move(board, start_row, start_col, end_row, end_col)) {
                return false;
            }
            break;
        case kBishopType:
            if (!is_bishop_move(board, start_row, start_col, end_row, end_col)) {
                return false;
            }
            break;
        case kQueenType:
            if (!is_queen_move(board, start_row, start_col, end_row, end_col)) {
                return false;
            }
            break;
        case kKnightType:
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
