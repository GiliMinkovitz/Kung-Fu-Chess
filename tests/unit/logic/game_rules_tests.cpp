#include "model/game_config.h"
#include "rules/game_rules.h"
#include "test_helpers.h"

#include <doctest/doctest.h>

TEST_CASE("GameRulesTest - StandardRulesConfiguration") {
    const kfc::GameRules rules = kfc::KungFuChessRules::standard();
    CHECK_EQ(rules.move_duration_ms, kfc::kMoveDurationMs);
    CHECK_EQ(rules.jump_duration_ms, kfc::kJumpDurationMs);
    CHECK(rules.is_game_over(kfc::test::make_piece(kfc::PieceColor::White, kfc::PieceKind::King)));
    CHECK(rules.is_game_over(kfc::test::make_piece(kfc::PieceColor::Black, kfc::PieceKind::King)));
    CHECK_FALSE(rules.is_game_over(kfc::test::make_piece(kfc::PieceColor::White, kfc::PieceKind::Pawn)));

    const kfc::Piece white_pawn =
        kfc::test::make_piece(kfc::PieceColor::White, kfc::PieceKind::Pawn);
    CHECK_EQ(rules.on_reach_last_row(white_pawn, 0, 8).kind, kfc::PieceKind::Queen);
    CHECK_EQ(rules.on_reach_last_row(white_pawn, 1, 8).kind, kfc::PieceKind::Pawn);
    CHECK_EQ(rules.on_reach_last_row(kfc::test::make_piece(kfc::PieceColor::White, kfc::PieceKind::Rook),
                                     0, 8)
                 .kind,
             kfc::PieceKind::Rook);

    const kfc::Piece black_pawn =
        kfc::test::make_piece(kfc::PieceColor::Black, kfc::PieceKind::Pawn);
    CHECK_EQ(rules.on_reach_last_row(black_pawn, 7, 8).kind, kfc::PieceKind::Queen);
}
