#include "model/game_config.h"
#include "rules/game_rules.h"
#include "server/match.h"
#include "test_helpers.h"

#include <doctest/doctest.h>

TEST_CASE("MatchTest - SubmitsActionsAndTicksClock") {
    kfc::Match match(kfc::test::make_board({{"wK", ".", "bK"}}));

    match.submit_action(kfc::Select{0, 0});
    CHECK(match.state().has_selection());

    match.tick(250);
    CHECK_EQ(match.state().clock_ms(), 250);
    CHECK_FALSE(match.is_game_over());
}

TEST_CASE("MatchTest - UsesCustomRulesConstructor") {
    const kfc::GameRules rules = kfc::KungFuChessRules::standard();
    kfc::Match match(kfc::test::make_board({{"wK", ".", "bK"}}), rules);

    CHECK_EQ(match.state().rows(), 1u);
    CHECK_EQ(match.state().cols(), 3u);
}

TEST_CASE("MatchTest - ReportsGameOverAfterKingCapture") {
    kfc::Match match(kfc::test::make_board({{"wR", ".", "bK"}, {"wK", ".", "."}}));

    match.submit_action(kfc::Select{0, 0});
    match.submit_action(kfc::MoveSelected{0, 2});
    match.tick(3 * kfc::kMoveDurationMs);

    CHECK(match.is_game_over());
    REQUIRE(match.state().winning_color().has_value());
    CHECK(*match.state().winning_color() == kfc::PieceColor::White);
}
