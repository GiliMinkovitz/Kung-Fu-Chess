#pragma once

#include "../../core/board_model.h"

namespace kfc::piece_rules {

[[nodiscard]] bool is_pawn_move(const BoardModel& board, int start_row, int start_col,
                                int end_row, int end_col);

}  // namespace kfc::piece_rules
