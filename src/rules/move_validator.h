#pragma once

#include "../model/board_model.h"
#include "../model/piece.h"

namespace kfc {

// Single entry point for static move legality on a board snapshot. Dispatches to
// piece_rules via RulesRegistry; does not consider pending moves, jumps, or route
// conflicts (those belong to GameState / RealTimeArbiter at request time).
[[nodiscard]] bool is_legal_move(const BoardModel& board, PieceKind kind, int start_row,
                                 int start_col, int end_row, int end_col);

}  // namespace kfc
