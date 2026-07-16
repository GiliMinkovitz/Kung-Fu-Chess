#include "model/board_model.h"
#include "io/game_input_handler.h"
#include "logic/game_state.h"
#include "test_helpers.h"

#include <doctest/doctest.h>
#include <sstream>

TEST_CASE("GameInputHandlerTest - GameInputHandlerClickSelectAndMove") {
    kfc::BoardModel board = kfc::test::make_board({{"wK", ".", "bK"}, {".", ".", "."}});
    kfc::GameState state(board);
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    CHECK(state.has_selection());
    std::size_t row = 0;
    std::size_t col = 0;
    REQUIRE(state.selection(row, col));
    CHECK_EQ(row, 0u);
    CHECK_EQ(col, 0u);

    processor.execute("click 150 150", sink);
    CHECK_FALSE(state.has_selection());
    CHECK_EQ(state.token_at(0, 0), "wK");
    CHECK_EQ(state.token_at(1, 1), ".");

    processor.execute("wait 1000", sink);
    CHECK_EQ(state.token_at(0, 0), ".");
    CHECK_EQ(state.token_at(1, 1), "wK");
}

TEST_CASE("GameInputHandlerTest - GameInputHandlerClickOutsideGridIgnored") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 350 50", sink);
    processor.execute("click -10 50", sink);
    CHECK_FALSE(state.has_selection());
}

TEST_CASE("GameInputHandlerTest - GameInputHandlerFriendlyClickReplacesSelection") {
    kfc::BoardModel board = kfc::test::make_board({{"wK", "wN", "bK"}});
    kfc::GameState state(board);
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 150 50", sink);
    CHECK(state.has_selection());
    std::size_t row = 0;
    std::size_t col = 0;
    REQUIRE(state.selection(row, col));
    CHECK_EQ(row, 0u);
    CHECK_EQ(col, 1u);
    CHECK_EQ(state.token_at(0, 0), "wK");
    CHECK_EQ(state.token_at(0, 1), "wN");
}

TEST_CASE("GameInputHandlerTest - GameInputHandlerPrintBoard") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    kfc::GameInputHandler processor(state);

    std::ostringstream output;
    processor.execute("print board", output);
    CHECK_EQ(output.str(), "wK . bK\n");
}

TEST_CASE("GameInputHandlerTest - GameInputHandlerCapture") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "bK"}});
    kfc::GameState state(board);
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    CHECK(state.has_selection());

    processor.execute("click 250 50", sink);
    CHECK_FALSE(state.has_selection());
    CHECK_EQ(state.token_at(0, 0), "wR");
    CHECK_EQ(state.token_at(0, 2), "bK");

    processor.execute("wait 2000", sink);
    CHECK_EQ(state.token_at(0, 0), ".");
    CHECK_EQ(state.token_at(0, 2), "wR");
}

TEST_CASE("GameInputHandlerTest - GameInputHandlerRejectsIllegalMove") {
    kfc::BoardModel board = kfc::test::make_board({{"wK", ".", ".", "."}, {".", ".", ".", "."}});
    kfc::GameState state(board);
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    CHECK(state.has_selection());

    processor.execute("click 350 50", sink);
    CHECK(state.has_selection());
    CHECK_EQ(state.token_at(0, 0), "wK");
    CHECK_EQ(state.token_at(0, 3), ".");
}

TEST_CASE("GameInputHandlerTest - PendingMovePrintBeforeArrival") {
    kfc::BoardModel board = kfc::test::make_board({{"wK", ".", "bK"}, {".", ".", "."}});
    kfc::GameState state(board);
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 150 150", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    CHECK_EQ(output.str(), "wK . bK\n. . .\n");
}

TEST_CASE("GameInputHandlerTest - PendingMovePrintAfterArrival") {
    kfc::BoardModel board = kfc::test::make_board({{"wK", ".", "bK"}, {".", ".", "."}});
    kfc::GameState state(board);
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 150 150", sink);
    processor.execute("wait 1000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    CHECK_EQ(output.str(), ". . bK\n. wK .\n");
}

TEST_CASE("GameInputHandlerTest - TwoCellMoveBeforeAndAfterArrival") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "."}});
    kfc::GameState state(board);
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    processor.execute("wait 1000", sink);

    std::ostringstream first_print;
    processor.execute("print board", first_print);
    CHECK_EQ(first_print.str(), "wR . .\n");

    processor.execute("wait 1000", sink);

    std::ostringstream second_print;
    processor.execute("print board", second_print);
    CHECK_EQ(second_print.str(), ". . wR\n");
}

TEST_CASE("GameInputHandlerTest - ClickOnMovingPieceDoesNotSelectOrRedirect") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "."}});
    kfc::GameState state(board);
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    processor.execute("wait 1000", sink);

    processor.execute("click 50 50", sink);
    CHECK_FALSE(state.has_selection());

    processor.execute("click 150 50", sink);
    processor.execute("wait 1000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    CHECK_EQ(output.str(), ". . wR\n");
}

TEST_CASE("GameInputHandlerTest - PieceCanMoveAfterLongRestExpires") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", ".", "."}});
    kfc::GameState state(board);
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 150 50", sink);
    processor.execute("wait 1000", sink);
    CHECK_FALSE(state.is_piece_moving(0, 1));
    CHECK(state.is_piece_resting(0, 1));

    processor.execute("wait 1000", sink);
    CHECK_FALSE(state.is_piece_resting(0, 1));

    processor.execute("click 150 50", sink);
    CHECK(state.has_selection());

    processor.execute("click 250 50", sink);
    processor.execute("wait 1000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    CHECK_EQ(output.str(), ". . wR .\n");
}

TEST_CASE("GameInputHandlerTest - OppositeColorsDoNotMoveConcurrentlyInCommonRoute") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "."}, {".", ".", "."}, {"bR", ".", "."}});
    kfc::GameState state(board);
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    processor.execute("click 50 250", sink);
    processor.execute("click 250 250", sink);
    processor.execute("wait 2000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    CHECK_EQ(output.str(), ". . wR\n. . .\nbR . .\n");
}

TEST_CASE("GameInputHandlerTest - OppositeColorsCanMoveOnDisjointRoutes") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", ".", "."}, {".", ".", ".", "."}, {".", ".", "bR", "."}});
    kfc::GameState state(board);
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 150 50", sink);
    processor.execute("click 250 250", sink);
    processor.execute("click 350 250", sink);
    processor.execute("wait 1000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    CHECK_EQ(output.str(), ". wR . .\n. . . .\n. . . bR\n");
}

TEST_CASE("GameInputHandlerTest - MovingPieceIgnoresRedirect") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "."}});
    kfc::GameState state(board);
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    processor.execute("wait 1000", sink);
    processor.execute("click 50 50", sink);
    processor.execute("click 150 50", sink);
    processor.execute("wait 1000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    CHECK_EQ(output.str(), ". . wR\n");
}

TEST_CASE("GameInputHandlerTest - RejectsMoveForPieceAlreadyInPendingMove") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "."}});
    kfc::GameState state(board);
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    CHECK(state.is_piece_moving(0, 0));

    processor.execute("click 50 50", sink);
    CHECK_FALSE(state.has_selection());

    processor.execute("click 150 50", sink);
    CHECK_FALSE(state.has_selection());

    processor.execute("wait 1000", sink);
    CHECK(state.is_piece_moving(0, 0));

    processor.execute("wait 1000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    CHECK_EQ(output.str(), ". . wR\n");
}

TEST_CASE("GameInputHandlerTest - RejectsTwoSameColorMovesToSameSquare") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", ".", "."}, {".", ".", ".", "."}, {".", ".", ".", "wN"}});
    kfc::GameState state(board);
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    processor.execute("click 350 250", sink);
    processor.execute("click 250 50", sink);
    processor.execute("wait 2000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    CHECK_EQ(output.str(), ". . wR .\n. . . .\n. . . wN\n");
}

TEST_CASE("GameInputHandlerTest - KingCaptureSetsGameOver") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "bK"}});
    kfc::GameState state(board);
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    CHECK_FALSE(state.is_game_over());

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    CHECK_FALSE(state.is_game_over());

    processor.execute("wait 2000", sink);
    CHECK(state.is_game_over());
    CHECK_EQ(state.token_at(0, 0), ".");
    CHECK_EQ(state.token_at(0, 2), "wR");
}

TEST_CASE("GameInputHandlerTest - CommandsIgnoredAfterGameOver") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "bK"}, {".", "wN", "."}});
    kfc::GameState state(board);
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    processor.execute("wait 2000", sink);
    CHECK(state.is_game_over());

    const kfc::test::BoardLayout board_snapshot = kfc::test::capture_layout(state);
    const std::int64_t clock_snapshot = state.clock_ms();

    processor.execute("click 150 150", sink);
    CHECK_FALSE(state.has_selection());

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    CHECK_FALSE(state.has_selection());
    CHECK(kfc::test::layout_matches(state, board_snapshot));

    processor.execute("wait 5000", sink);
    CHECK_EQ(state.clock_ms(), clock_snapshot);
    CHECK(kfc::test::layout_matches(state, board_snapshot));
}

TEST_CASE("GameInputHandlerTest - PrintBoardAfterKingCapture") {
    kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "bK"}});
    kfc::GameState state(board);
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    processor.execute("wait 2000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    CHECK_EQ(output.str(), ". . wR\n");
}

TEST_CASE("GameInputHandlerTest - JumpCommandAirbornePieceCapturesArrivingEnemy") {
    kfc::BoardModel board = kfc::test::make_board({{".", ".", "."}, {"wK", ".", "bR"}, {".", ".", "."}});
    kfc::GameState state(board);
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("jump 50 150", sink);
    processor.execute("click 250 150", sink);
    processor.execute("click 50 150", sink);
    processor.execute("wait 1000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    CHECK_EQ(output.str(), ". . .\nwK . .\n. . .\n");
}

TEST_CASE("GameInputHandlerTest - GameInputHandlerWaitWithoutMs") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    CHECK_EQ(state.clock_ms(), 0);
    processor.execute("wait", sink);
    CHECK_EQ(state.clock_ms(), 0);
}

TEST_CASE("GameInputHandlerTest - GameInputHandlerUnknownVerb") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("foobar 1 2", sink);
    CHECK_FALSE(state.has_selection());
    CHECK_EQ(state.clock_ms(), 0);
}

TEST_CASE("GameInputHandlerTest - JumpCommandTooLateDoesNotSavePiece") {
    kfc::BoardModel board = kfc::test::make_board({{".", ".", "."}, {"wK", ".", "bR"}, {".", ".", "."}});
    kfc::GameState state(board);
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 250 150", sink);
    processor.execute("click 50 150", sink);
    processor.execute("wait 1000", sink);
    processor.execute("jump 50 150", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    CHECK_EQ(output.str(), ". . .\nbR . .\n. . .\n");
}

TEST_CASE("GameInputHandlerTest - GameInputHandlerClickWithoutCoords") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click", sink);
    CHECK_FALSE(state.has_selection());
}

TEST_CASE("GameInputHandlerTest - GameInputHandlerJumpWithoutCoords") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("jump", sink);
    CHECK_FALSE(state.is_piece_jumping(0, 0));
}

TEST_CASE("GameInputHandlerTest - GameInputHandlerPrintPartialCommandIgnored") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    kfc::GameInputHandler processor(state);
    std::ostringstream output;

    processor.execute("print", output);
    CHECK(output.str().empty());
    processor.execute("print foo", output);
    CHECK(output.str().empty());
}

TEST_CASE("GameInputHandlerTest - GameInputHandlerSelectEmptySquare") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 150 50", sink);
    CHECK_FALSE(state.has_selection());
}

TEST_CASE("GameInputHandlerTest - GameInputHandlerFriendlyClickSameCellJumps") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    CHECK(state.has_selection());
    processor.execute("click 50 50", sink);
    CHECK_FALSE(state.has_selection());
    CHECK(state.is_piece_jumping(0, 0));
}

TEST_CASE("GameInputHandlerTest - FriendlyClickSameCellWhileMovingIgnored") {
    kfc::GameState state(kfc::test::make_board({{"wR", ".", "."}}));
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    CHECK(state.is_piece_moving(0, 0));
    CHECK_FALSE(state.has_selection());

    state.select(0, 0);
    processor.execute("click 50 50", sink);
    CHECK(state.is_piece_moving(0, 0));
    CHECK_FALSE(state.is_piece_jumping(0, 0));
}

TEST_CASE("GameInputHandlerTest - FriendlyClickSameCellWhileJumpingIgnored") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 50 50", sink);
    REQUIRE(state.is_piece_jumping(0, 0));
    CHECK_FALSE(state.has_selection());

    state.select(0, 0);
    processor.execute("click 50 50", sink);
    CHECK(state.is_piece_jumping(0, 0));
    CHECK(state.has_selection());
}

TEST_CASE("GameInputHandlerTest - GameInputHandlerJumpOutsideGrid") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("jump 350 50", sink);
    CHECK_FALSE(state.is_piece_jumping(0, 0));
}

TEST_CASE("GameInputHandlerTest - GameInputHandlerJumpOnEmptyCell") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("jump 150 50", sink);
    CHECK_FALSE(state.is_piece_jumping(0, 1));
}

TEST_CASE("GameInputHandlerTest - ClickOnEmptyBoardIgnored") {
    kfc::GameState state(kfc::BoardModel{});
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    CHECK_FALSE(state.has_selection());
}

TEST_CASE("GameInputHandlerTest - JumpWhilePieceMovingIgnored") {
    kfc::GameState state(kfc::test::make_board({{"wR", ".", "."}}));
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    REQUIRE(state.is_piece_moving(0, 0));
    state.select(0, 0);
    processor.execute("jump 50 50", sink);
    CHECK(state.is_piece_moving(0, 0));
    CHECK_FALSE(state.is_piece_jumping(0, 0));
}

TEST_CASE("GameInputHandlerTest - MoveAttemptWhilePieceRestingIgnored") {
    kfc::GameState state(kfc::test::make_board({{"wR", ".", "bK"}}));
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 150 50", sink);
    processor.execute("wait 1000", sink);
    REQUIRE(state.is_piece_resting(0, 1));

    state.select(0, 1);
    processor.execute("click 250 50", sink);
    CHECK(state.is_piece_resting(0, 1));
    CHECK_EQ(state.token_at(0, 1), "wR");
    CHECK_EQ(state.token_at(0, 2), "bK");
}

TEST_CASE("GameInputHandlerTest - JumpWhilePieceRestingIgnored") {
    kfc::GameState state(kfc::test::make_board({{"wR", ".", "."}}));
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 150 50", sink);
    processor.execute("wait 1000", sink);
    REQUIRE(state.is_piece_resting(0, 1));

    processor.execute("jump 150 50", sink);
    CHECK_FALSE(state.is_piece_jumping(0, 1));
    CHECK(state.is_piece_resting(0, 1));
}

TEST_CASE("GameInputHandlerTest - MoveAttemptWhilePieceMovingIgnored") {
    kfc::GameState state(kfc::test::make_board({{"wR", ".", "bK"}}));
    kfc::GameInputHandler processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    REQUIRE(state.is_piece_moving(0, 0));
    state.select(0, 0);
    processor.execute("click 150 50", sink);
    CHECK(state.is_piece_moving(0, 0));
    CHECK(state.has_selection());
    CHECK_EQ(state.token_at(0, 0), "wR");
}
