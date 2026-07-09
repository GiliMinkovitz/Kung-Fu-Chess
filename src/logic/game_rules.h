#pragma once

#include "core/board_model.h"
#include "core/piece.h"

#include <cstdint>
#include <functional>

namespace kfc {

struct GameRules {
    std::function<bool(const BoardModel& board, PieceKind kind, int start_row, int start_col,
                       int end_row, int end_col)>
        is_legal_move;

    std::function<Piece(Piece piece, std::size_t end_row, std::size_t board_rows)> on_reach_last_row;

    std::function<bool(const Piece& lost_piece)> is_game_over;

    std::int64_t move_duration_ms = 1000;
    std::int64_t jump_duration_ms = 1000;
};

namespace KungFuChessRules {

[[nodiscard]] GameRules standard();

}  // namespace KungFuChessRules

}  // namespace kfc
