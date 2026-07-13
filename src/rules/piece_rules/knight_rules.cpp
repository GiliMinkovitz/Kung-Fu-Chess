#include "knight_rules.h"

#include <cstdlib>

namespace kfc::piece_rules {

namespace {

[[nodiscard]] bool is_knight_move(int dr, int dc) noexcept {
    const int adr = std::abs(dr);
    const int adc = std::abs(dc);
    return (adr == 2 && adc == 1) || (adr == 1 && adc == 2);
}

}  // namespace

bool validate_knight_move(const BoardModel& board, int start_row, int start_col, int end_row,
                          int end_col) {
    (void)board;
    const int dr = end_row - start_row;
    const int dc = end_col - start_col;
    return is_knight_move(dr, dc);
}

}  // namespace kfc::piece_rules
