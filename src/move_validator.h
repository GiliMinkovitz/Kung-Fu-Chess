#pragma once

#include "board.h"

namespace kfc {

[[nodiscard]] bool is_legal_move(const Board& board, char piece, int start_row, int start_col,
                                 int end_row, int end_col);

}  // namespace kfc
