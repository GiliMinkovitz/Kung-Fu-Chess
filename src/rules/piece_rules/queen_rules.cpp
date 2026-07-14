#include "queen_rules.h"

#include "bishop_rules.h"
#include "rook_rules.h"

namespace kfc::piece_rules {

bool is_queen_move(const BoardModel& board, int start_row, int start_col, int end_row,
                   int end_col) {
    return is_rook_move(board, start_row, start_col, end_row, end_col) ||
           is_bishop_move(board, start_row, start_col, end_row, end_col);
}

}  // namespace kfc::piece_rules
