#include "king_rules.h"

#include <cstdlib>

namespace kfc::piece_rules {

bool is_king_move(const BoardModel& board, int start_row, int start_col, int end_row,
                  int end_col) {
    (void)board;
    const int dr = end_row - start_row;
    const int dc = end_col - start_col;
    const int adr = std::abs(dr);
    const int adc = std::abs(dc);
    return adr <= 1 && adc <= 1 && (adr != 0 || adc != 0);
}

}  // namespace kfc::piece_rules
