#include "core/board_model.h"
#include "core/game_config.h"
#include "core/piece_factory.h"
#include "logic/game_rules.h"
#include "logic/real_time_arbiter.h"
#include "test_helpers.h"

#include <doctest/doctest.h>

namespace {

kfc::Piece::Id piece_id_at(const kfc::BoardModel& board, std::size_t row, std::size_t col) {
    const kfc::Piece* piece = board.piece_at(row, col);
    REQUIRE(piece != nullptr);
    return piece->id;
}

}  // namespace

TEST_CASE("RealTimeArbiterTest - RequestMoveSchedulesInTransitState") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "."}});
    kfc::RealTimeArbiter arbiter;
    const auto piece_id = piece_id_at(board, 0, 0);

    arbiter.request_move(piece_id, kfc::PieceColor::White, {0, 0}, {0, 2}, kfc::kMoveDurationMs);

    CHECK(arbiter.is_piece_moving(0, 0));
    CHECK_FALSE(arbiter.is_piece_moving(0, 2));
}

TEST_CASE("RealTimeArbiterTest - RequestJumpSchedulesJumpState") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "."}});
    kfc::RealTimeArbiter arbiter;
    const auto piece_id = piece_id_at(board, 0, 0);

    arbiter.request_jump(piece_id, kfc::PieceColor::White, {0, 0}, kfc::kJumpDurationMs);

    CHECK(arbiter.is_piece_jumping(0, 0));
    CHECK_FALSE(arbiter.is_piece_moving(0, 0));
}

TEST_CASE("RealTimeArbiterTest - RequestMoveSettlesOnBoardAfterDuration") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "."}});
    kfc::RealTimeArbiter arbiter;
    kfc::GameRules rules = kfc::KungFuChessRules::standard();
    bool game_over = false;
    const auto piece_id = piece_id_at(board, 0, 0);

    board.get_piece(piece_id).state = kfc::PieceState::Moving;
    arbiter.request_move(piece_id, kfc::PieceColor::White, {0, 0}, {0, 2},
                         2 * kfc::kMoveDurationMs);

    arbiter.update_time(2 * kfc::kMoveDurationMs, board, rules, game_over);

    CHECK_FALSE(arbiter.is_piece_moving(0, 0));
    CHECK_EQ(board.token_at(0, 0), ".");
    CHECK_EQ(board.token_at(0, 2), "wR");
}

TEST_CASE("RealTimeArbiterTest - RequestJumpExpiresAfterDuration") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "."}});
    kfc::RealTimeArbiter arbiter;
    kfc::GameRules rules = kfc::KungFuChessRules::standard();
    bool game_over = false;
    const auto piece_id = piece_id_at(board, 0, 0);

    arbiter.request_jump(piece_id, kfc::PieceColor::White, {0, 0}, kfc::kJumpDurationMs);

    CHECK(arbiter.is_piece_jumping(0, 0));

    arbiter.update_time(kfc::kJumpDurationMs, board, rules, game_over);

    CHECK_FALSE(arbiter.is_piece_jumping(0, 0));
}

TEST_CASE("RealTimeArbiterTest - MoveAbortedIfFriendlyOccupiesTargetBeforeArrival") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", ".", "."}});
    kfc::RealTimeArbiter arbiter;
    kfc::GameRules rules = kfc::KungFuChessRules::standard();
    bool game_over = false;
    const auto piece_id = piece_id_at(board, 0, 0);

    board.get_piece(piece_id).state = kfc::PieceState::Moving;
    arbiter.request_move(piece_id, kfc::PieceColor::White, {0, 0}, {0, 2}, 2 * kfc::kMoveDurationMs);

    kfc::PieceFactory factory(board.next_piece_id());
    board.place_piece_at(0, 2,
                          factory.create(kfc::PieceColor::White, kfc::PieceKind::King, {0, 2}));

    arbiter.update_time(2 * kfc::kMoveDurationMs, board, rules, game_over);

    CHECK_EQ(board.token_at(0, 0), "wR");
    CHECK_EQ(board.token_at(0, 2), "wK");
}

TEST_CASE("RealTimeArbiterTest - WouldConflictWithOppositeColorMove") {
    kfc::BoardModel board = kfc::test::make_board({{".", ".", "."}});
    kfc::RealTimeArbiter arbiter;
    kfc::GameRules rules = kfc::KungFuChessRules::standard();
    bool game_over = false;

    arbiter.request_move(2, kfc::PieceColor::Black, {0, 2}, {0, 0}, kfc::kMoveDurationMs);

    CHECK(arbiter.would_conflict_with_opposite_color_move(kfc::PieceColor::White, 1, {0, 0}, {0, 2},
                                                          kfc::kMoveDurationMs));

    arbiter.update_time(kfc::kMoveDurationMs, board, rules, game_over);

    CHECK_FALSE(arbiter.would_conflict_with_opposite_color_move(
        kfc::PieceColor::White, 1, {0, 0}, {0, 2}, kfc::kMoveDurationMs));
}

TEST_CASE("RealTimeArbiterTest - SameColorDestinationClaimed") {
    kfc::RealTimeArbiter arbiter;

    arbiter.request_move(1, kfc::PieceColor::White, {0, 0}, {0, 2}, kfc::kMoveDurationMs);

    CHECK(arbiter.is_same_color_destination_claimed(kfc::PieceColor::White, {0, 2}));
    CHECK_FALSE(arbiter.is_same_color_destination_claimed(kfc::PieceColor::Black, {0, 2}));
}
