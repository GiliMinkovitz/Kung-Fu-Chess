#include "game_rules.h"

#include "move_validator.h"

namespace kfc {
namespace KungFuChessRules {

namespace {

[[nodiscard]] Piece promote_pawn_to_queen(Piece piece, std::size_t end_row,
                                          std::size_t board_rows) {
    if (piece.type != 'P') {
        return piece;
    }

    const std::size_t last_row = board_rows - 1;
    if (piece.color == 'w' && end_row == 0) {
        return Piece{'w', 'Q'};
    }
    if (piece.color == 'b' && end_row == last_row) {
        return Piece{'b', 'Q'};
    }
    return piece;
}

[[nodiscard]] bool king_capture_ends_game(Piece lost_piece) {
    return lost_piece.type == 'K';
}

}  // namespace

GameRules standard() {
    GameRules rules;
    rules.is_legal_move = is_legal_move;
    rules.on_reach_last_row = promote_pawn_to_queen;
    rules.is_game_over = king_capture_ends_game;
    rules.move_duration_ms = 1000;
    rules.jump_duration_ms = 1000;
    return rules;
}

}  // namespace KungFuChessRules
}  // namespace kfc
