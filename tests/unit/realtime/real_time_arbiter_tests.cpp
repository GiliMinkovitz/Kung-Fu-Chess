#include "model/board_model.h"
#include "model/game_config.h"
#include "model/piece_factory.h"
#include "rules/game_rules.h"
#include "realtime/real_time_arbiter.h"
#include "realtime/render_snapshot.h"
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

    arbiter.request_move(piece_id, kfc::PieceColor::White, kfc::PieceKind::Rook, {0, 0}, {0, 2}, kfc::kMoveDurationMs);

    CHECK(arbiter.is_piece_moving(0, 0));
    CHECK_FALSE(arbiter.is_piece_moving(0, 2));
}

TEST_CASE("RealTimeArbiterTest - RequestJumpSchedulesJumpState") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "."}});
    kfc::RealTimeArbiter arbiter;
    const auto piece_id = piece_id_at(board, 0, 0);

    arbiter.request_jump(piece_id, kfc::PieceColor::White, kfc::PieceKind::Rook, {0, 0},
                         kfc::kJumpDurationMs);

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
    arbiter.request_move(piece_id, kfc::PieceColor::White, kfc::PieceKind::Rook, {0, 0}, {0, 2},
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

    arbiter.request_jump(piece_id, kfc::PieceColor::White, kfc::PieceKind::Rook, {0, 0},
                         kfc::kJumpDurationMs);

    CHECK(arbiter.is_piece_jumping(0, 0));

    arbiter.update_time(kfc::kJumpDurationMs, board, rules, game_over);

    CHECK_FALSE(arbiter.is_piece_jumping(0, 0));
}

TEST_CASE("RealTimeArbiterTest - MoveSettlesWhenStartCellClearedBeforeArrival") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "."}});
    kfc::RealTimeArbiter arbiter;
    kfc::GameRules rules = kfc::KungFuChessRules::standard();
    bool game_over = false;
    const auto piece_id = piece_id_at(board, 0, 0);

    board.get_piece(piece_id).state = kfc::PieceState::Moving;
    arbiter.request_move(piece_id, kfc::PieceColor::White, kfc::PieceKind::Rook, {0, 0}, {0, 2}, 2 * kfc::kMoveDurationMs);

    board.clear_cell(0, 0);

    arbiter.update_time(2 * kfc::kMoveDurationMs, board, rules, game_over);

    CHECK_FALSE(arbiter.is_piece_moving(0, 0));
    CHECK_EQ(board.token_at(0, 0), ".");
    CHECK_EQ(board.token_at(0, 2), "wR");
    CHECK_EQ(board.get_piece(piece_id).state, kfc::PieceState::Idle);
}

TEST_CASE("RealTimeArbiterTest - MoveAbortedIfFriendlyOccupiesTargetBeforeArrival") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", ".", "."}});
    kfc::RealTimeArbiter arbiter;
    kfc::GameRules rules = kfc::KungFuChessRules::standard();
    bool game_over = false;
    const auto piece_id = piece_id_at(board, 0, 0);

    board.get_piece(piece_id).state = kfc::PieceState::Moving;
    arbiter.request_move(piece_id, kfc::PieceColor::White, kfc::PieceKind::Rook, {0, 0}, {0, 2}, 2 * kfc::kMoveDurationMs);

    kfc::PieceFactory factory(board.next_piece_id());
    board.place_piece_at(0, 2,
                          factory.create(kfc::PieceColor::White, kfc::PieceKind::King, {0, 2}));

    arbiter.update_time(2 * kfc::kMoveDurationMs, board, rules, game_over);

    CHECK_FALSE(arbiter.is_piece_moving(0, 0));
    CHECK_EQ(board.token_at(0, 0), "wR");
    CHECK_EQ(board.token_at(0, 2), "wK");
    CHECK_EQ(board.get_piece(piece_id).state, kfc::PieceState::Idle);
}

TEST_CASE("RealTimeArbiterTest - WouldConflictWithOppositeColorMove") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "bR"}});
    kfc::RealTimeArbiter arbiter;
    kfc::GameRules rules = kfc::KungFuChessRules::standard();
    bool game_over = false;
    const auto white_id = piece_id_at(board, 0, 0);
    const auto black_id = piece_id_at(board, 0, 2);

    arbiter.request_move(black_id, kfc::PieceColor::Black, kfc::PieceKind::Rook, {0, 2}, {0, 0}, kfc::kMoveDurationMs);

    CHECK(arbiter.would_conflict_with_opposite_color_move(kfc::PieceColor::White, white_id, {0, 0},
                                                          {0, 2}, kfc::kMoveDurationMs));

    arbiter.update_time(kfc::kMoveDurationMs, board, rules, game_over);

    CHECK_FALSE(arbiter.would_conflict_with_opposite_color_move(
        kfc::PieceColor::White, white_id, {0, 0}, {0, 2}, kfc::kMoveDurationMs));
}

TEST_CASE("RealTimeArbiterTest - SameColorDestinationClaimed") {
    kfc::RealTimeArbiter arbiter;

    arbiter.request_move(1, kfc::PieceColor::White, kfc::PieceKind::Rook, {0, 0}, {0, 2}, kfc::kMoveDurationMs);

    CHECK(arbiter.is_same_color_destination_claimed(kfc::PieceColor::White, {0, 2}));
    CHECK_FALSE(arbiter.is_same_color_destination_claimed(kfc::PieceColor::Black, {0, 2}));
}

TEST_CASE("RealTimeArbiterTest - AnimationsForRenderTracksMoveProgress") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "."}});
    kfc::RealTimeArbiter arbiter;
    kfc::GameRules rules = kfc::KungFuChessRules::standard();
    bool game_over = false;
    const auto piece_id = piece_id_at(board, 0, 0);

    arbiter.request_move(piece_id, kfc::PieceColor::White, kfc::PieceKind::Rook, {0, 0}, {0, 2},
                         2 * kfc::kMoveDurationMs);
    arbiter.update_time(kfc::kMoveDurationMs, board, rules, game_over);

    const kfc::AnimationSnapshot snapshot = arbiter.animations_for_render();
    REQUIRE(snapshot.moves.size() == 1);
    CHECK_EQ(snapshot.moves[0].piece_id, piece_id);
    CHECK(snapshot.moves[0].kind == kfc::PieceKind::Rook);
    CHECK(snapshot.moves[0].color == kfc::PieceColor::White);
    CHECK_EQ(snapshot.moves[0].from_row, 0);
    CHECK_EQ(snapshot.moves[0].from_col, 0);
    CHECK_EQ(snapshot.moves[0].to_row, 0);
    CHECK_EQ(snapshot.moves[0].to_col, 2);
    CHECK(snapshot.moves[0].progress == doctest::Approx(0.5f));
    CHECK(snapshot.jumps.empty());
}

TEST_CASE("RealTimeArbiterTest - JumpSettlesWhenStartCellClearedBeforeExpiry") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "."}});
    kfc::RealTimeArbiter arbiter;
    kfc::GameRules rules = kfc::KungFuChessRules::standard();
    bool game_over = false;
    const auto piece_id = piece_id_at(board, 0, 0);

    board.get_piece(piece_id).state = kfc::PieceState::Moving;
    arbiter.request_jump(piece_id, kfc::PieceColor::White, kfc::PieceKind::Rook, {0, 0},
                         kfc::kJumpDurationMs);

    board.clear_cell(0, 0);

    arbiter.update_time(kfc::kJumpDurationMs, board, rules, game_over);

    CHECK_FALSE(arbiter.is_piece_jumping(0, 0));
    CHECK_EQ(board.token_at(0, 0), "wR");
    CHECK_EQ(board.get_piece(piece_id).state, kfc::PieceState::Idle);
}

TEST_CASE("RealTimeArbiterTest - JumpRestoresOverOccupiedCellOnExpiry") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "."}});
    kfc::RealTimeArbiter arbiter;
    kfc::GameRules rules = kfc::KungFuChessRules::standard();
    bool game_over = false;
    const auto rook_id = piece_id_at(board, 0, 0);

    board.get_piece(rook_id).state = kfc::PieceState::Moving;
    arbiter.request_jump(rook_id, kfc::PieceColor::White, kfc::PieceKind::Rook, {0, 0},
                         kfc::kJumpDurationMs);

    board.clear_cell(0, 0);

    kfc::PieceFactory factory(board.next_piece_id());
    board.place_piece_at(0, 0,
                          factory.create(kfc::PieceColor::Black, kfc::PieceKind::King, {0, 0}));
    const auto occupant_id = board.piece_at(0, 0)->id;

    arbiter.update_time(kfc::kJumpDurationMs, board, rules, game_over);

    CHECK_FALSE(arbiter.is_piece_jumping(0, 0));
    CHECK_EQ(board.token_at(0, 0), "wR");
    CHECK_EQ(board.piece_at(0, 0)->id, rook_id);
    CHECK_EQ(board.get_piece(rook_id).state, kfc::PieceState::Idle);
    CHECK_EQ(board.get_piece(occupant_id).state, kfc::PieceState::Captured);
}

TEST_CASE("RealTimeArbiterTest - AnimationsForRenderTracksJumpProgress") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "."}});
    kfc::RealTimeArbiter arbiter;
    kfc::GameRules rules = kfc::KungFuChessRules::standard();
    bool game_over = false;
    const auto piece_id = piece_id_at(board, 0, 0);

    arbiter.request_jump(piece_id, kfc::PieceColor::White, kfc::PieceKind::Rook, {0, 0},
                         kfc::kJumpDurationMs);
    arbiter.update_time(kfc::kJumpDurationMs / 2, board, rules, game_over);

    const kfc::AnimationSnapshot snapshot = arbiter.animations_for_render();
    REQUIRE(snapshot.jumps.size() == 1);
    CHECK_EQ(snapshot.jumps[0].piece_id, piece_id);
    CHECK(snapshot.jumps[0].kind == kfc::PieceKind::Rook);
    CHECK(snapshot.jumps[0].color == kfc::PieceColor::White);
    CHECK_EQ(snapshot.jumps[0].row, 0);
    CHECK_EQ(snapshot.jumps[0].col, 0);
    CHECK(snapshot.jumps[0].progress == doctest::Approx(0.5f));
    CHECK(snapshot.moves.empty());
}

TEST_CASE("RealTimeArbiterTest - RequestMoveSchedulesLongRestAfterSettlement") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "."}});
    kfc::RealTimeArbiter arbiter;
    kfc::GameRules rules = kfc::KungFuChessRules::standard();
    bool game_over = false;
    const auto piece_id = piece_id_at(board, 0, 0);

    board.get_piece(piece_id).state = kfc::PieceState::Moving;
    arbiter.request_move(piece_id, kfc::PieceColor::White, kfc::PieceKind::Rook, {0, 0}, {0, 1}, kfc::kMoveDurationMs);

    arbiter.update_time(kfc::kMoveDurationMs, board, rules, game_over);

    CHECK(arbiter.is_piece_resting(piece_id));
    arbiter.update_time(kfc::kLongRestDurationMs, board, rules, game_over);
    CHECK_FALSE(arbiter.is_piece_resting(piece_id));
}

TEST_CASE("RealTimeArbiterTest - RequestJumpSchedulesShortRestAfterCompletion") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "."}});
    kfc::RealTimeArbiter arbiter;
    kfc::GameRules rules = kfc::KungFuChessRules::standard();
    bool game_over = false;
    const auto piece_id = piece_id_at(board, 0, 0);

    arbiter.request_jump(piece_id, kfc::PieceColor::White, kfc::PieceKind::Rook, {0, 0},
                         kfc::kJumpDurationMs);

    arbiter.update_time(kfc::kJumpDurationMs, board, rules, game_over);

    CHECK(arbiter.is_piece_resting(piece_id));
    arbiter.update_time(kfc::kShortRestDurationMs, board, rules, game_over);
    CHECK_FALSE(arbiter.is_piece_resting(piece_id));
}

TEST_CASE("RealTimeArbiterTest - AbortedMoveRestoresOverOccupiedStartCell") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", ".", "."}});
    kfc::RealTimeArbiter arbiter;
    kfc::GameRules rules = kfc::KungFuChessRules::standard();
    bool game_over = false;
    const auto rook_id = piece_id_at(board, 0, 0);

    board.get_piece(rook_id).state = kfc::PieceState::Moving;
    arbiter.request_move(rook_id, kfc::PieceColor::White, kfc::PieceKind::Rook, {0, 0}, {0, 3}, 3 * kfc::kMoveDurationMs);

    board.clear_cell(0, 0);

    kfc::PieceFactory factory(board.next_piece_id());
    board.place_piece_at(0, 2,
                          factory.create(kfc::PieceColor::White, kfc::PieceKind::Pawn, {0, 2}));
    const auto occupant_id = board.next_piece_id();
    board.place_piece_at(0, 0,
                          factory.create(kfc::PieceColor::Black, kfc::PieceKind::King, {0, 0}));

    arbiter.update_time(3 * kfc::kMoveDurationMs, board, rules, game_over);

    CHECK_FALSE(arbiter.is_piece_resting(rook_id));
    CHECK_EQ(board.get_piece(rook_id).state, kfc::PieceState::Idle);
    CHECK_EQ(board.token_at(0, 0), "wR");
    CHECK_EQ(board.piece_at(0, 0)->id, rook_id);
    CHECK_EQ(board.get_piece(occupant_id).state, kfc::PieceState::Captured);
    CHECK(kfc::test::BoardModelTestAccess::find_piece_by_id(board, occupant_id) != nullptr);
    CHECK_EQ(board.token_at(0, 2), "wP");
    CHECK_EQ(board.token_at(0, 3), ".");
}

TEST_CASE("RealTimeArbiterTest - AbortedMoveDoesNotScheduleRest") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", ".", "."}});
    kfc::RealTimeArbiter arbiter;
    kfc::GameRules rules = kfc::KungFuChessRules::standard();
    bool game_over = false;
    const auto piece_id = piece_id_at(board, 0, 0);

    board.get_piece(piece_id).state = kfc::PieceState::Moving;
    arbiter.request_move(piece_id, kfc::PieceColor::White, kfc::PieceKind::Rook, {0, 0}, {0, 3}, 3 * kfc::kMoveDurationMs);

    board.clear_cell(0, 0);

    kfc::PieceFactory factory(board.next_piece_id());
    board.place_piece_at(0, 2,
                          factory.create(kfc::PieceColor::White, kfc::PieceKind::Pawn, {0, 2}));

    arbiter.update_time(3 * kfc::kMoveDurationMs, board, rules, game_over);

    CHECK_FALSE(arbiter.is_piece_resting(piece_id));
    CHECK_EQ(board.get_piece(piece_id).state, kfc::PieceState::Idle);
    CHECK_EQ(board.token_at(0, 0), "wR");
    CHECK_EQ(board.token_at(0, 3), ".");
}

TEST_CASE("RealTimeArbiterTest - AnimationsForRenderEmptyAfterArrival") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "."}});
    kfc::RealTimeArbiter arbiter;
    kfc::GameRules rules = kfc::KungFuChessRules::standard();
    bool game_over = false;
    const auto piece_id = piece_id_at(board, 0, 0);

    board.get_piece(piece_id).state = kfc::PieceState::Moving;
    arbiter.request_move(piece_id, kfc::PieceColor::White, kfc::PieceKind::Rook, {0, 0}, {0, 2}, kfc::kMoveDurationMs);
    arbiter.update_time(kfc::kMoveDurationMs, board, rules, game_over);

    const kfc::AnimationSnapshot snapshot = arbiter.animations_for_render();
    CHECK(snapshot.moves.empty());
    CHECK(snapshot.jumps.empty());
}

TEST_CASE("RealTimeArbiterTest - CancelsInvalidInFlightMoveWhenFriendlySettlesOnDestination") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", "wK", "."}});
    kfc::RealTimeArbiter arbiter;
    kfc::GameRules rules = kfc::KungFuChessRules::standard();
    bool game_over = false;
    const auto rook_id = piece_id_at(board, 0, 0);
    const auto king_id = piece_id_at(board, 0, 1);

    board.get_piece(rook_id).state = kfc::PieceState::Moving;
    board.get_piece(king_id).state = kfc::PieceState::Moving;
    board.clear_cell(0, 0);
    board.clear_cell(0, 1);

    arbiter.request_move(rook_id, kfc::PieceColor::White, kfc::PieceKind::Rook, {0, 0}, {0, 2},
                         2 * kfc::kMoveDurationMs);
    arbiter.request_move(king_id, kfc::PieceColor::White, kfc::PieceKind::King, {0, 1}, {0, 2},
                         kfc::kMoveDurationMs);

    arbiter.update_time(kfc::kMoveDurationMs, board, rules, game_over);

    CHECK_EQ(board.token_at(0, 0), "wR");
    CHECK_EQ(board.token_at(0, 1), ".");
    CHECK_EQ(board.token_at(0, 2), "wK");
    CHECK_EQ(board.get_piece(rook_id).state, kfc::PieceState::Idle);
    CHECK_FALSE(arbiter.is_piece_moving(0, 0));
    CHECK(arbiter.animations_for_render().moves.empty());
}

TEST_CASE("RealTimeArbiterTest - CancelsInvalidInFlightMoveWhenPathBlockedBySettlement") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", "wR", ".", "."}});
    kfc::RealTimeArbiter arbiter;
    kfc::GameRules rules = kfc::KungFuChessRules::standard();
    bool game_over = false;
    const auto rook_a_id = piece_id_at(board, 0, 0);
    const auto rook_b_id = piece_id_at(board, 0, 1);

    board.get_piece(rook_a_id).state = kfc::PieceState::Moving;
    board.get_piece(rook_b_id).state = kfc::PieceState::Moving;
    board.clear_cell(0, 0);
    board.clear_cell(0, 1);

    arbiter.request_move(rook_a_id, kfc::PieceColor::White, kfc::PieceKind::Rook, {0, 0}, {0, 3},
                         3 * kfc::kMoveDurationMs);
    arbiter.request_move(rook_b_id, kfc::PieceColor::White, kfc::PieceKind::Rook, {0, 1}, {0, 2},
                         kfc::kMoveDurationMs);

    arbiter.update_time(kfc::kMoveDurationMs, board, rules, game_over);

    CHECK_EQ(board.token_at(0, 0), "wR");
    CHECK_EQ(board.token_at(0, 2), "wR");
    CHECK_EQ(board.token_at(0, 3), ".");
    CHECK_EQ(board.get_piece(rook_a_id).state, kfc::PieceState::Idle);
    CHECK_FALSE(arbiter.is_piece_moving(0, 0));
    CHECK(arbiter.animations_for_render().moves.empty());
}

TEST_CASE("RealTimeArbiterTest - KeepsValidInFlightMoveAfterUnrelatedSettlement") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", ".", "wR"}});
    kfc::RealTimeArbiter arbiter;
    kfc::GameRules rules = kfc::KungFuChessRules::standard();
    bool game_over = false;
    const auto rook_a_id = piece_id_at(board, 0, 0);
    const auto rook_b_id = piece_id_at(board, 0, 3);

    board.get_piece(rook_a_id).state = kfc::PieceState::Moving;
    board.get_piece(rook_b_id).state = kfc::PieceState::Moving;
    board.clear_cell(0, 0);
    board.clear_cell(0, 3);

    arbiter.request_move(rook_a_id, kfc::PieceColor::White, kfc::PieceKind::Rook, {0, 0}, {0, 1},
                         2 * kfc::kMoveDurationMs);
    arbiter.request_move(rook_b_id, kfc::PieceColor::White, kfc::PieceKind::Rook, {0, 3}, {0, 2},
                         kfc::kMoveDurationMs);

    arbiter.update_time(kfc::kMoveDurationMs, board, rules, game_over);

    CHECK_EQ(board.token_at(0, 2), "wR");
    CHECK(arbiter.is_piece_moving(0, 0));
    CHECK_EQ(arbiter.animations_for_render().moves.size(), 1u);
    CHECK_EQ(arbiter.animations_for_render().moves.front().piece_id, rook_a_id);
}

TEST_CASE("RealTimeArbiterTest - AbortsIllegalPawnMoveAtSettlement") {
    kfc::BoardModel board = kfc::test::make_board({{".", ".", "."}, {"wP", ".", "."}});
    kfc::RealTimeArbiter arbiter;
    kfc::GameRules rules = kfc::KungFuChessRules::standard();
    bool game_over = false;
    const auto pawn_id = piece_id_at(board, 1, 0);

    board.get_piece(pawn_id).state = kfc::PieceState::Moving;
    board.clear_cell(1, 0);
    arbiter.request_move(pawn_id, kfc::PieceColor::White, kfc::PieceKind::Pawn, {1, 0}, {1, 1},
                         kfc::kMoveDurationMs);

    arbiter.update_time(kfc::kMoveDurationMs, board, rules, game_over);

    CHECK_FALSE(arbiter.is_piece_moving(1, 0));
    CHECK_EQ(board.token_at(1, 0), "wP");
    CHECK_EQ(board.get_piece(pawn_id).state, kfc::PieceState::Idle);
}

TEST_CASE("RealTimeArbiterTest - AbortsMoveForUnknownPieceKindAtSettlement") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "."}});
    kfc::RealTimeArbiter arbiter;
    kfc::GameRules rules = kfc::KungFuChessRules::standard();
    bool game_over = false;
    const auto piece_id = piece_id_at(board, 0, 0);

    board.get_piece(piece_id).kind = kfc::PieceKind::Count;
    board.get_piece(piece_id).state = kfc::PieceState::Moving;
    board.clear_cell(0, 0);
    arbiter.request_move(piece_id, kfc::PieceColor::White, kfc::PieceKind::Count, {0, 0}, {0, 2},
                         kfc::kMoveDurationMs);

    arbiter.update_time(kfc::kMoveDurationMs, board, rules, game_over);

    CHECK_FALSE(arbiter.is_piece_moving(0, 0));
    REQUIRE(board.piece_at(0, 0) != nullptr);
    CHECK_EQ(board.piece_at(0, 0)->id, piece_id);
    CHECK_EQ(board.get_piece(piece_id).state, kfc::PieceState::Idle);
}
