#pragma once

#include "core/board_model.h"
#include "core/piece.h"

namespace kfc {

[[nodiscard]] bool is_legal_move(const BoardModel& board, PieceKind kind, int start_row,
                                 int start_col, int end_row, int end_col);

}  // namespace kfc
