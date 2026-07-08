#include "board_model.h"
#include "collision_resolver.h"
#include "game_config.h"
#include "game_rules.h"
#include "piece.h"
#include "test_helpers.h"

#include <doctest/doctest.h>
#include <vector>
#include <optional>

TEST_CASE("CollisionResolverTest - CollisionHasCommonRouteHorizontalParallel") {
    const kfc::PendingMove left_to_right{
        *kfc::Piece::from_token("wR"), {0, 0}, {0, 4}, 1000};
    const kfc::PendingMove middle_to_right{
        *kfc::Piece::from_token("bR"), {0, 2}, {0, 6}, 1000};
    CHECK(kfc::CollisionResolver::has_common_route(left_to_right, middle_to_right));
}

TEST_CASE("CollisionResolverTest - CollisionHasCommonRouteVerticalParallel") {
    const kfc::PendingMove top_to_bottom{
        *kfc::Piece::from_token("wR"), {0, 0}, {4, 0}, 1000};
    const kfc::PendingMove middle_to_bottom{
        *kfc::Piece::from_token("bR"), {2, 0}, {6, 0}, 1000};
    CHECK(kfc::CollisionResolver::has_common_route(top_to_bottom, middle_to_bottom));
}

TEST_CASE("CollisionResolverTest - CollisionHasCommonRouteDisjoint") {
    const kfc::PendingMove left{
        *kfc::Piece::from_token("wR"), {0, 0}, {0, 1}, 1000};
    const kfc::PendingMove right{
        *kfc::Piece::from_token("bR"), {0, 3}, {0, 4}, 1000};
    CHECK_FALSE(kfc::CollisionResolver::has_common_route(left, right));
}

TEST_CASE("CollisionResolverTest - CollisionConflictsWithOppositeColorMove") {
    std::vector<kfc::PendingMove> pending;
    pending.push_back({*kfc::Piece::from_token("bR"), {0, 2}, {0, 0}, 1000});
    const kfc::PendingMove proposed{*kfc::Piece::from_token("wR"), {0, 0}, {0, 2}, 1000};
    CHECK(kfc::CollisionResolver::conflicts_with_opposite_color_move(pending, 500, 'w', proposed));
    CHECK_FALSE(kfc::CollisionResolver::conflicts_with_opposite_color_move(pending, 1500, 'w', proposed));
}

TEST_CASE("CollisionResolverTest - CollisionSameColorDestinationClaimed") {
    std::vector<kfc::PendingMove> pending;
    pending.push_back({*kfc::Piece::from_token("wN"), {0, 0}, {0, 2}, 1000});
    CHECK(kfc::CollisionResolver::is_same_color_destination_claimed(pending, 500, 'w', {0, 2}));
    CHECK_FALSE(kfc::CollisionResolver::is_same_color_destination_claimed(pending, 500, 'b', {0, 2}));
    CHECK_FALSE(kfc::CollisionResolver::is_same_color_destination_claimed(pending, 1500, 'w', {0, 2}));
}

TEST_CASE("CollisionResolverTest - CollisionNormalCaptureOnArrival") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "bP"}});
    kfc::CollisionResolver resolver;
    const kfc::GameRules rules = kfc::KungFuChessRules::standard();
    bool game_over = false;
    const kfc::ArrivingPieceInfo arriving{
        *kfc::Piece::from_token("wR"), {0, 0}, {0, 2}};
    CHECK(resolver.check_for_jump_capture(board, rules, 1000, {}, {0, 2}, arriving, game_over));
    CHECK_EQ(board.token_at(0, 2), "wR");
    CHECK_FALSE(game_over);
}

TEST_CASE("CollisionResolverTest - CollisionEmptyDestinationNotCaptured") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "."}});
    kfc::CollisionResolver resolver;
    const kfc::GameRules rules = kfc::KungFuChessRules::standard();
    bool game_over = false;
    const kfc::ArrivingPieceInfo arriving{
        *kfc::Piece::from_token("wR"), {0, 0}, {0, 2}};
    CHECK_FALSE(resolver.check_for_jump_capture(board, rules, 1000, {}, {0, 2}, arriving, game_over));
    CHECK_EQ(board.token_at(0, 0), "wR");
}

TEST_CASE("CollisionResolverTest - CollisionJumpCaptureSetsGameOver") {
    kfc::BoardModel board = kfc::test::make_board({{".", ".", "."}, {"wK", ".", "."}, {".", ".", "."}});
    kfc::CollisionResolver resolver;
    const kfc::GameRules rules = kfc::KungFuChessRules::standard();
    std::vector<kfc::JumpState> jumps;
    jumps.push_back({*kfc::Piece::from_token("wK"), {1, 0}, 2000});
    bool game_over = false;
    const kfc::ArrivingPieceInfo arriving{
        *kfc::Piece::from_token("bK"), {2, 0}, {1, 0}};
    CHECK(resolver.check_for_jump_capture(board, rules, 1000, jumps, {1, 0}, arriving, game_over));
    CHECK(game_over);
}
