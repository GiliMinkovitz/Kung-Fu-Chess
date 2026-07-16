#include "game_rules.h"

#include "../model/game_config.h"
#include "move_validator.h"

namespace kfc {
namespace KungFuChessRules {

namespace {

[[nodiscard]] Piece promote_pawn_to_queen(Piece piece, std::size_t end_row,
                                          std::size_t board_rows) {
    if (piece.kind != PieceKind::Pawn) {
        return piece;
    }

    const std::size_t last_row = board_rows - 1;
    if (piece.is_white() && end_row == 0) {
        piece.kind = PieceKind::Queen;
    } else if (piece.is_black() && end_row == last_row) {
        piece.kind = PieceKind::Queen;
    }
    return piece;
}

[[nodiscard]] bool king_capture_ends_game(const Piece& lost_piece) {
    return lost_piece.kind == PieceKind::King;
}

}  // namespace

GameRules standard() {
    GameRules rules;
    rules.is_legal_move = is_legal_move;
    rules.on_reach_last_row = promote_pawn_to_queen;
    rules.is_game_over = king_capture_ends_game;
    rules.move_duration_ms = kMoveDurationMs;
    rules.jump_duration_ms = kJumpDurationMs;
    rules.long_rest_duration_ms = kLongRestDurationMs;
    rules.short_rest_duration_ms = kShortRestDurationMs;
    return rules;
}

}  // namespace KungFuChessRules
}  // namespace kfc
