#include "model/board_model.h"
#include "model/game_config.h"
#include "model/piece_factory.h"
#include "rules/game_rules.h"
#include "logic/game_state.h"
#include "rules/move_validator.h"
#include "test_helpers.h"

#include <doctest/doctest.h>
#include <optional>

TEST_CASE("GameStateTest - GameStateWaitIncrementsClock") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    CHECK_EQ(state.clock_ms(), 0);
    state.add_clock(250);
    state.add_clock(50);
    CHECK_EQ(state.clock_ms(), 300);
}

TEST_CASE("GameStateTest - PawnPromotionToQueen") {
    kfc::BoardModel white_board = kfc::test::make_board({{".", ".", "."}, {".", "wP", "."}});
    kfc::GameState white_state(white_board);
    white_state.select(1, 1);
    white_state.move_selected_to(0, 1);
    white_state.add_clock(1000);
    CHECK_EQ(white_state.token_at(0, 1), "wQ");
    CHECK_EQ(white_state.token_at(1, 1), ".");

    kfc::BoardModel black_board = kfc::test::make_board({{".", "bP", "."}, {".", ".", "."}});
    kfc::GameState black_state(black_board);
    black_state.select(0, 1);
    black_state.move_selected_to(1, 1);
    black_state.add_clock(1000);
    CHECK_EQ(black_state.token_at(1, 1), "bQ");
    CHECK_EQ(black_state.token_at(0, 1), ".");
}

TEST_CASE("GameStateTest - IsPieceMovingWhileInTransit") {
    kfc::GameState state(kfc::test::make_board({{"wR", ".", "."}}));
    state.select(0, 0);
    state.move_selected_to(0, 2);
    CHECK(state.is_piece_moving(0, 0));
    CHECK_FALSE(state.is_piece_moving(0, 1));
    CHECK_FALSE(state.is_piece_moving(0, 2));
}

TEST_CASE("GameStateTest - IsPieceMovingFalseAfterSettleNoCooldown") {
    kfc::GameState state(kfc::test::make_board({{"wR", ".", "."}}));
    state.select(0, 0);
    state.move_selected_to(0, 2);
    CHECK(state.is_piece_moving(0, 0));

    state.add_clock(2000);
    CHECK_FALSE(state.is_piece_moving(0, 0));
    CHECK_FALSE(state.is_piece_moving(0, 2));
    CHECK(state.is_selectable_piece(0, 2));
}

TEST_CASE("GameStateTest - JumpCaptureInterceptsArrivingEnemy") {
    kfc::BoardModel board = kfc::test::make_board({{".", ".", "."}, {"wR", ".", "."}, {"bR", ".", "."}});
    kfc::GameState state(board);

    state.select(2, 0);
    state.move_selected_to(1, 0);
    state.add_clock(500);

    state.select(1, 0);
    state.jump_selected();
    CHECK(state.is_piece_jumping(1, 0));

    state.add_clock(500);

    CHECK_EQ(state.token_at(2, 0), ".");
    CHECK_EQ(state.token_at(1, 0), "wR");
}

TEST_CASE("GameStateTest - MovingPieceCannotJump") {
    kfc::GameState state(kfc::test::make_board({{"wR", ".", "."}}));
    state.select(0, 0);
    state.move_selected_to(0, 2);
    CHECK(state.is_piece_moving(0, 0));

    state.select(0, 0);
    state.jump_selected();
    CHECK_FALSE(state.is_piece_jumping(0, 0));

    state.add_clock(2000);
    CHECK_EQ(state.token_at(0, 2), "wR");
}

TEST_CASE("GameStateTest - JumpStatusClearedAfterDuration") {
    kfc::GameState state(kfc::test::make_board({{"wR", ".", "."}}));
    state.select(0, 0);
    state.jump_selected();
    CHECK(state.is_piece_jumping(0, 0));

    state.add_clock(1000);
    CHECK_FALSE(state.is_piece_jumping(0, 0));
    CHECK_EQ(state.token_at(0, 0), "wR");
}

TEST_CASE("GameStateTest - GameStateCustomRules") {
    kfc::GameRules rules;
    rules.is_legal_move = kfc::is_legal_move;
    rules.on_reach_last_row = [](kfc::Piece piece, std::size_t, std::size_t) { return piece; };
    rules.is_game_over = [](const kfc::Piece&) { return false; };
    rules.move_duration_ms = 500;
    rules.jump_duration_ms = 300;

    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "."}});
    kfc::GameState state(board, rules);
    state.select(0, 0);
    state.move_selected_to(0, 2);
    CHECK(state.is_piece_moving(0, 0));

    state.add_clock(1000);
    CHECK_EQ(state.token_at(0, 2), "wR");
    CHECK_FALSE(state.is_game_over());

    state.select(0, 2);
    state.jump_selected();
    CHECK(state.is_piece_jumping(0, 2));
    state.add_clock(300);
    CHECK_FALSE(state.is_piece_jumping(0, 2));
}

TEST_CASE("GameStateTest - GameStateSelectOutOfBounds") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    state.select(99, 99);
    CHECK_FALSE(state.has_selection());
}

TEST_CASE("GameStateTest - GameStateMoveAndJumpWithoutSelection") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    state.move_selected_to(0, 1);
    state.jump_selected();
    CHECK_EQ(state.token_at(0, 0), "wK");
    CHECK_FALSE(state.is_piece_jumping(0, 0));
}

TEST_CASE("GameStateTest - GameStateJumpAtEmptyCell") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    state.jump_at(0, 1);
    CHECK_FALSE(state.is_piece_jumping(0, 1));
}

TEST_CASE("GameStateTest - GameStateIsPieceOutOfBounds") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    CHECK_FALSE(state.is_piece(99, 99));
}

TEST_CASE("GameStateTest - GameStateFriendlySelectionRequiresSelection") {
    kfc::GameState state(kfc::test::make_board({{"wK", "wN", "bK"}}));
    CHECK_FALSE(state.is_friendly_to_selection(0, 1));
}

TEST_CASE("GameStateTest - FriendlySelectionReturnsTrueForSameColor") {
    kfc::GameState state(kfc::test::make_board({{"wK", "wN", "bK"}}));
    state.select(0, 0);
    CHECK(state.is_friendly_to_selection(0, 1));
    CHECK_FALSE(state.is_friendly_to_selection(0, 2));
}

TEST_CASE("GameStateTest - IsLegalMoveFromEmptySquare") {
    const kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    CHECK_FALSE(kfc::test::GameStateTestAccess::is_legal_move(state, 0, 1, 0, 2));
}

TEST_CASE("GameStateTest - MoveSelectedFromEmptyCellIgnored") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    state.select(0, 1);
    state.move_selected_to(0, 2);
    CHECK(state.has_selection());
    CHECK_EQ(state.token_at(0, 1), ".");
    CHECK_EQ(state.token_at(0, 2), "bK");
}

TEST_CASE("GameStateTest - JumpAtStaleCellIdIgnored") {
    kfc::BoardModel board = kfc::test::make_board({{"."}});
    kfc::test::BoardModelTestAccess::set_cell_piece_id(board, 0, 0, 999);
    kfc::GameState state(std::move(board));
    state.jump_at(0, 0);
    CHECK_FALSE(state.is_piece_jumping(0, 0));
}

TEST_CASE("GameStateTest - GameStateSamekfc::test::BoardLayoutAs") {
    const kfc::GameState left(kfc::test::make_board({{"wK", ".", "bK"}}));
    const kfc::GameState right(kfc::test::make_board({{"wK", ".", "bK"}}));
    const kfc::GameState different(kfc::test::make_board({{"wK", ".", "."}}));
    CHECK(left.same_board_layout_as(right));
    CHECK_FALSE(left.same_board_layout_as(different));
}

TEST_CASE("GameStateTest - GameStateCaptureMoveUsesSingleCellDuration") {
    kfc::GameState state(kfc::test::make_board({{"wR", ".", "bP"}}));
    state.select(0, 0);
    state.move_selected_to(0, 2);
    state.add_clock(999);
    CHECK_EQ(state.token_at(0, 0), "wR");
    CHECK_EQ(state.token_at(0, 2), "bP");
    state.add_clock(1);
    CHECK_EQ(state.token_at(0, 0), ".");
    CHECK_EQ(state.token_at(0, 2), "wR");
}

TEST_CASE("GameStateTest - SelectionReturnsFalseWhenNone") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    std::size_t row = 99;
    std::size_t col = 99;
    CHECK_FALSE(state.selection(row, col));
}

TEST_CASE("GameStateTest - FriendlySelectionRequiresOccupiedSelectedCell") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "wN"}}));
    state.select(0, 1);
    CHECK_FALSE(state.is_friendly_to_selection(0, 2));
}

TEST_CASE("GameStateTest - MoveSelectedToOutOfBoundsIgnored") {
    kfc::GameState state(kfc::test::make_board({{"wR", ".", "."}}));
    state.select(0, 0);
    state.move_selected_to(99, 99);
    CHECK(state.has_selection());
    CHECK_EQ(state.token_at(0, 0), "wR");
}

TEST_CASE("GameStateTest - MoveSelectedToWhileMovingIgnored") {
    kfc::GameState state(kfc::test::make_board({{"wR", ".", "."}}));
    state.select(0, 0);
    state.move_selected_to(0, 2);
    state.select(0, 0);
    state.move_selected_to(0, 1);
    CHECK(state.is_piece_moving(0, 0));
    CHECK_EQ(state.token_at(0, 0), "wR");
}

TEST_CASE("GameStateTest - AbortedMoveResetsPieceStateToIdle") {
    kfc::GameState state(kfc::test::make_board({{"wR", ".", ".", "."}}));
    const kfc::Piece::Id piece_id =
        kfc::test::GameStateTestAccess::board(state).piece_at(0, 0)->id;

    state.select(0, 0);
    state.move_selected_to(0, 2);
    CHECK_EQ(kfc::test::GameStateTestAccess::piece_state(state, piece_id),
             kfc::PieceState::Moving);

    kfc::BoardModel& board = kfc::test::GameStateTestAccess::board(state);
    kfc::PieceFactory factory(board.next_piece_id());
    board.place_piece_at(0, 2,
                          factory.create(kfc::PieceColor::White, kfc::PieceKind::King, {0, 2}));

    state.add_clock(2 * kfc::kMoveDurationMs);

    CHECK_FALSE(state.is_piece_moving(0, 0));
    CHECK_EQ(kfc::test::GameStateTestAccess::piece_state(state, piece_id), kfc::PieceState::Idle);
    CHECK_EQ(state.token_at(0, 0), "wR");
    CHECK_EQ(state.token_at(0, 2), "wK");
}
