#include "core/game_config.h"
#include "logic/game_rules.h"
#include "core/piece.h"

#include <doctest/doctest.h>
#include <optional>

TEST_CASE("GameRulesTest - StandardRulesConfiguration") {
    const kfc::GameRules rules = kfc::KungFuChessRules::standard();
    CHECK_EQ(rules.move_duration_ms, kfc::kMoveDurationMs);
    CHECK_EQ(rules.jump_duration_ms, kfc::kJumpDurationMs);
    CHECK(rules.is_game_over(*kfc::Piece::from_token("wK")));
    CHECK(rules.is_game_over(*kfc::Piece::from_token("bK")));
    CHECK_FALSE(rules.is_game_over(*kfc::Piece::from_token("wP")));

    const kfc::Piece white_pawn = *kfc::Piece::from_token("wP");
    CHECK_EQ(rules.on_reach_last_row(white_pawn, 0, 8).type, kfc::kQueenType);
    CHECK_EQ(rules.on_reach_last_row(white_pawn, 1, 8).type, kfc::kPawnType);
    CHECK_EQ(rules.on_reach_last_row(*kfc::Piece::from_token("wR"), 0, 8).type, kfc::kRookType);

    const kfc::Piece black_pawn = *kfc::Piece::from_token("bP");
    CHECK_EQ(rules.on_reach_last_row(black_pawn, 7, 8).type, kfc::kQueenType);
}
