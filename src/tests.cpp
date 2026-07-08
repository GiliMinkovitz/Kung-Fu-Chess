#include "board_model.h"
#include "board_validator.h"
#include "board_writer.h"
#include "collision_resolver.h"
#include "command_processor.h"
#include "game_config.h"
#include "game_rules.h"
#include "game_state.h"
#include "move_validator.h"
#include "path_utils.h"
#include "piece.h"
#include "vpl_io.h"

#include <doctest/doctest.h>
#include <sstream>
#include <string>
#include <vector>

namespace {

kfc::BoardModel make_board(std::initializer_list<std::initializer_list<const char*>> rows) {
    return kfc::BoardModel::from_token_grid(rows);
}

using BoardLayout = std::vector<std::vector<std::string>>;

BoardLayout capture_layout(const kfc::GameState& state) {
    BoardLayout layout(state.rows(), std::vector<std::string>(state.cols()));
    for (std::size_t row = 0; row < state.rows(); ++row) {
        for (std::size_t col = 0; col < state.cols(); ++col) {
            layout[row][col] = state.token_at(row, col);
        }
    }
    return layout;
}

bool layout_matches(const kfc::GameState& state, const BoardLayout& layout) {
    if (state.rows() != layout.size()) {
        return false;
    }
    for (std::size_t row = 0; row < state.rows(); ++row) {
        if (state.cols() != layout[row].size()) {
            return false;
        }
        for (std::size_t col = 0; col < state.cols(); ++col) {
            if (state.token_at(row, col) != layout[row][col]) {
                return false;
            }
        }
    }
    return true;
}

TEST_CASE("BoardModelTest - InvalidEmptyBoard") {
    const kfc::BoardModel board;
    CHECK_FALSE(board.is_valid());
}

TEST_CASE("BoardModelTest - InvalidNonRectangularBoard") {
    const kfc::BoardModel board = make_board({{"wK", "."}, {"bK"}});
    CHECK_FALSE(board.is_valid());
}

TEST_CASE("BoardValidatorTest - InvalidUnknownToken") {
    CHECK_FALSE(kfc::is_valid_token("xZ"));
    CHECK(kfc::is_valid_token("."));
    CHECK(kfc::is_valid_token("wK"));
    CHECK(kfc::is_valid_token("bQ"));
}

TEST_CASE("BoardValidatorTest - ParseBoardRowsSuccess") {
    const std::vector<std::string> lines = {"wK . . bK", ". . . .", "wR . . bR"};
    kfc::BoardModel board;
    CHECK_EQ(kfc::parse_board_rows(lines, board), kfc::BoardError::Ok);
    CHECK_EQ(board.rows(), 3);
    CHECK_EQ(board.cols(), 4);
    CHECK_EQ(board.token_at(0, 0), "wK");
    CHECK_EQ(board.token_at(2, 3), "bR");
}

TEST_CASE("BoardValidatorTest - ParseBoardUnknownToken") {
    const std::vector<std::string> lines = {"wK xZ", ". ."};
    kfc::BoardModel board;
    CHECK_EQ(kfc::parse_board_rows(lines, board), kfc::BoardError::UnknownToken);
}

TEST_CASE("BoardValidatorTest - ParseBoardRowWidthMismatch") {
    const std::vector<std::string> lines = {"wK . .", ". bK"};
    kfc::BoardModel board;
    CHECK_EQ(kfc::parse_board_rows(lines, board), kfc::BoardError::RowWidthMismatch);
}

TEST_CASE("VplIoTest - ReadAndWriteRoundtrip") {
    const std::string input =
        "Board:\n"
        "wK . bQ\n"
        ". wN .\n"
        "bP . wR\n"
        "Commands:\n"
        "print board\n";

    std::istringstream in(input);
    const kfc::VplInput parsed = kfc::read_vpl_input(in);
    CHECK_EQ(parsed.error, kfc::BoardError::Ok);
    CHECK(parsed.board.is_valid());
    CHECK_EQ(parsed.commands.size(), 1);
    CHECK_EQ(parsed.commands[0], "print board");

    std::ostringstream output;
    kfc::write_board(output, parsed.board);
    CHECK_EQ(output.str(), "wK . bQ\n. wN .\nbP . wR");
}

TEST_CASE("VplIoTest - InvalidBoardProducesError") {
    const std::string input =
        "Board:\n"
        "wK xZ\n"
        ". .\n"
        "Commands:\n";

    std::istringstream in(input);
    const kfc::VplInput parsed = kfc::read_vpl_input(in);
    CHECK_EQ(parsed.error, kfc::BoardError::UnknownToken);
    CHECK_EQ(std::string(kfc::board_error_message(parsed.error)), kfc::kErrorUnknownToken);
}

TEST_CASE("GameStateTest - GameStateWaitIncrementsClock") {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    CHECK_EQ(state.clock_ms(), 0);
    state.add_clock(250);
    state.add_clock(50);
    CHECK_EQ(state.clock_ms(), 300);
}

TEST_CASE("CommandProcessorTest - CommandProcessorClickSelectAndMove") {
    kfc::BoardModel board = make_board({{"wK", ".", "bK"}, {".", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
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

TEST_CASE("CommandProcessorTest - CommandProcessorClickOutsideGridIgnored") {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 350 50", sink);
    processor.execute("click -10 50", sink);
    CHECK_FALSE(state.has_selection());
}

TEST_CASE("CommandProcessorTest - CommandProcessorFriendlyClickReplacesSelection") {
    kfc::BoardModel board = make_board({{"wK", "wN", "bK"}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
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

TEST_CASE("CommandProcessorTest - CommandProcessorPrintBoard") {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    kfc::CommandProcessor processor(state);

    std::ostringstream output;
    processor.execute("print board", output);
    CHECK_EQ(output.str(), "wK . bK");
}

TEST_CASE("MoveValidatorTest - KnightLegalLMove") {
    const kfc::BoardModel board = make_board({{".", ".", ".", ".", "."},
                              {".", ".", ".", ".", "."},
                              {".", ".", "wN", ".", "."},
                              {".", ".", ".", ".", "."},
                              {".", ".", ".", ".", "."}});
    CHECK(kfc::is_legal_move(board, 'N', 2, 2, 4, 3));
    CHECK(kfc::is_legal_move(board, 'N', 2, 2, 0, 3));
    CHECK_FALSE(kfc::is_legal_move(board, 'N', 2, 2, 2, 4));
}

TEST_CASE("MoveValidatorTest - RookCannotMoveDiagonally") {
    const kfc::BoardModel board = make_board({{".", ".", "."}, {".", "wR", "."}, {".", ".", "."}});
    CHECK_FALSE(kfc::is_legal_move(board, 'R', 1, 1, 2, 2));
    CHECK_FALSE(kfc::is_legal_move(board, 'R', 1, 1, 0, 0));
    CHECK(kfc::is_legal_move(board, 'R', 1, 1, 1, 0));
    CHECK(kfc::is_legal_move(board, 'R', 1, 1, 2, 1));
}

TEST_CASE("MoveValidatorTest - KingCannotMoveMoreThanOneSquare") {
    const kfc::BoardModel board = make_board({{".", ".", ".", "."}, {".", "wK", ".", "."}, {".", ".", ".", "."}});
    CHECK_FALSE(kfc::is_legal_move(board, 'K', 1, 1, 1, 3));
    CHECK_FALSE(kfc::is_legal_move(board, 'K', 1, 1, 3, 1));
    CHECK_FALSE(kfc::is_legal_move(board, 'K', 1, 1, 3, 3));
    CHECK(kfc::is_legal_move(board, 'K', 1, 1, 1, 2));
    CHECK(kfc::is_legal_move(board, 'K', 1, 1, 2, 2));
}

TEST_CASE("MoveValidatorTest - MoveRespectsBoardBoundaries") {
    const kfc::BoardModel board = make_board({{"wN", ".", "wR"}, {".", "wK", "."}});
    CHECK_FALSE(kfc::is_legal_move(board, 'N', 0, 0, -1, 1));
    CHECK_FALSE(kfc::is_legal_move(board, 'R', 0, 2, 0, 5));
    CHECK_FALSE(kfc::is_legal_move(board, 'K', 1, 1, 2, 3));
}

TEST_CASE("MoveValidatorTest - RookBlockedByFriendlyPiece") {
    const kfc::BoardModel board = make_board({{".", ".", ".", "."}, {".", "wR", "wP", "."}});
    CHECK_FALSE(kfc::is_legal_move(board, 'R', 1, 1, 1, 3));
    CHECK(kfc::is_legal_move(board, 'R', 1, 1, 1, 0));
}

TEST_CASE("MoveValidatorTest - RookCapturesEnemyPiece") {
    const kfc::BoardModel board = make_board({{".", ".", ".", "."}, {".", "wR", ".", "bP"}});
    CHECK(kfc::is_legal_move(board, 'R', 1, 1, 1, 3));
}

TEST_CASE("MoveValidatorTest - KnightJumpsOverPieces") {
    const kfc::BoardModel board = make_board({{".", ".", ".", ".", "."},
                              {".", "wP", "bN", ".", "."},
                              {".", ".", "wN", ".", "."},
                              {".", ".", ".", ".", "."},
                              {".", ".", ".", ".", "."}});
    CHECK(kfc::is_legal_move(board, 'N', 2, 2, 0, 3));
    CHECK(kfc::is_legal_move(board, 'N', 2, 2, 4, 1));
}

TEST_CASE("MoveValidatorTest - CannotCaptureOwnPiece") {
    const kfc::BoardModel board = make_board({{".", "wP", "wR"}});
    CHECK_FALSE(kfc::is_legal_move(board, 'R', 0, 2, 0, 1));
    CHECK_FALSE(kfc::is_legal_move(board, 'N', 0, 2, 0, 1));
}

TEST_CASE("CommandProcessorTest - CommandProcessorCapture") {
    kfc::BoardModel board = make_board({{"wR", ".", "bK"}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
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

TEST_CASE("MoveValidatorTest - WhitePawnForwardMove") {
    const kfc::BoardModel board = make_board({{".", ".", "."}, {".", "wP", "."}, {".", ".", "."}});
    CHECK(kfc::is_legal_move(board, 'P', 1, 1, 0, 1));
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 1, 1, 0, 0));
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 1, 1, 0, 2));
}

TEST_CASE("MoveValidatorTest - WhitePawnBlockedForward") {
    const kfc::BoardModel board = make_board({{".", "bK", "."}, {".", "wP", "."}});
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 1, 1, 0, 1));
}

TEST_CASE("MoveValidatorTest - WhitePawnDiagonalCapture") {
    const kfc::BoardModel board = make_board({{"bN", ".", "bR"}, {".", "wP", "."}, {".", ".", "."}});
    CHECK(kfc::is_legal_move(board, 'P', 1, 1, 0, 0));
    CHECK(kfc::is_legal_move(board, 'P', 1, 1, 0, 2));

    const kfc::BoardModel empty_diagonal = make_board({{".", ".", "."}, {".", "wP", "."}});
    CHECK_FALSE(kfc::is_legal_move(empty_diagonal, 'P', 1, 1, 0, 0));
    CHECK_FALSE(kfc::is_legal_move(empty_diagonal, 'P', 1, 1, 0, 2));
}

TEST_CASE("MoveValidatorTest - WhitePawnCannotMoveBackwardOrForwardCapture") {
    const kfc::BoardModel board = make_board({{".", "wP", "."}, {".", "bK", "."}, {".", ".", "."}});
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 0, 1, 1, 1));
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 0, 1, 1, 0));
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 0, 1, 1, 2));
}

TEST_CASE("MoveValidatorTest - WhitePawnCannotCaptureOwnPiece") {
    const kfc::BoardModel board = make_board({{"wN", ".", "wR"}, {".", "wP", "."}});
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 1, 1, 0, 0));
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 1, 1, 0, 2));
}

TEST_CASE("MoveValidatorTest - BlackPawnForwardMove") {
    const kfc::BoardModel board = make_board({{".", ".", "."}, {".", "bP", "."}, {".", ".", "."}});
    CHECK(kfc::is_legal_move(board, 'P', 1, 1, 2, 1));
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 1, 1, 2, 0));
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 1, 1, 2, 2));
}

TEST_CASE("MoveValidatorTest - BlackPawnBlockedForward") {
    const kfc::BoardModel board = make_board({{".", "bP", "."}, {".", "wK", "."}});
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 0, 1, 1, 1));
}

TEST_CASE("MoveValidatorTest - BlackPawnDiagonalCapture") {
    const kfc::BoardModel board = make_board({{".", ".", "."}, {".", "bP", "."}, {"wR", ".", "wN"}});
    CHECK(kfc::is_legal_move(board, 'P', 1, 1, 2, 0));
    CHECK(kfc::is_legal_move(board, 'P', 1, 1, 2, 2));

    const kfc::BoardModel empty_diagonal = make_board({{".", "bP", "."}, {".", ".", "."}});
    CHECK_FALSE(kfc::is_legal_move(empty_diagonal, 'P', 0, 1, 1, 0));
    CHECK_FALSE(kfc::is_legal_move(empty_diagonal, 'P', 0, 1, 1, 2));
}

TEST_CASE("MoveValidatorTest - BlackPawnCannotMoveBackwardOrForwardCapture") {
    const kfc::BoardModel board = make_board({{".", ".", "."}, {".", "wK", "."}, {".", "bP", "."}});
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 2, 1, 1, 1));
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 2, 1, 1, 0));
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 2, 1, 1, 2));
}

TEST_CASE("MoveValidatorTest - PawnDoubleMoveFromStartRow") {
    const kfc::BoardModel white_board = make_board({{".", ".", "."},
                                    {".", ".", "."},
                                    {".", ".", "."},
                                    {".", "wP", "."}});
    CHECK(kfc::is_legal_move(white_board, 'P', 3, 1, 1, 1));
    CHECK_FALSE(kfc::is_legal_move(white_board, 'P', 2, 1, 0, 1));

    const kfc::BoardModel black_board = make_board({{".", "bP", "."},
                                    {".", ".", "."},
                                    {".", ".", "."},
                                    {".", ".", "."}});
    CHECK(kfc::is_legal_move(black_board, 'P', 0, 1, 2, 1));
    CHECK_FALSE(kfc::is_legal_move(black_board, 'P', 1, 1, 3, 1));
}

TEST_CASE("MoveValidatorTest - PawnDoubleMoveBlockedByIntermediatePiece") {
    const kfc::BoardModel blocked_intermediate = make_board({{".", "bP", "."},
                                             {".", "wN", "."},
                                             {".", ".", "."},
                                             {".", ".", "."}});
    CHECK_FALSE(kfc::is_legal_move(blocked_intermediate, 'P', 0, 1, 2, 1));

    const kfc::BoardModel blocked_dest = make_board({{".", "bP", "."},
                                     {".", ".", "."},
                                     {".", "wR", "."},
                                     {".", ".", "."}});
    CHECK_FALSE(kfc::is_legal_move(blocked_dest, 'P', 0, 1, 2, 1));
}

TEST_CASE("GameStateTest - PawnPromotionToQueen") {
    kfc::BoardModel white_board = make_board({{".", ".", "."}, {".", "wP", "."}});
    kfc::GameState white_state(white_board);
    white_state.select(1, 1);
    white_state.move_selected_to(0, 1);
    white_state.add_clock(1000);
    CHECK_EQ(white_state.token_at(0, 1), "wQ");
    CHECK_EQ(white_state.token_at(1, 1), ".");

    kfc::BoardModel black_board = make_board({{".", "bP", "."}, {".", ".", "."}});
    kfc::GameState black_state(black_board);
    black_state.select(0, 1);
    black_state.move_selected_to(1, 1);
    black_state.add_clock(1000);
    CHECK_EQ(black_state.token_at(1, 1), "bQ");
    CHECK_EQ(black_state.token_at(0, 1), ".");
}

TEST_CASE("CommandProcessorTest - CommandProcessorRejectsIllegalMove") {
    kfc::BoardModel board = make_board({{"wK", ".", ".", "."}, {".", ".", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    CHECK(state.has_selection());

    processor.execute("click 350 50", sink);
    CHECK(state.has_selection());
    CHECK_EQ(state.token_at(0, 0), "wK");
    CHECK_EQ(state.token_at(0, 3), ".");
}

TEST_CASE("CommandProcessorTest - PendingMovePrintBeforeArrival") {
    kfc::BoardModel board = make_board({{"wK", ".", "bK"}, {".", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 150 150", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    CHECK_EQ(output.str(), "wK . bK\n. . .");
}

TEST_CASE("CommandProcessorTest - PendingMovePrintAfterArrival") {
    kfc::BoardModel board = make_board({{"wK", ".", "bK"}, {".", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 150 150", sink);
    processor.execute("wait 1000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    CHECK_EQ(output.str(), ". . bK\n. wK .");
}

TEST_CASE("CommandProcessorTest - TwoCellMoveBeforeAndAfterArrival") {
    kfc::BoardModel board = make_board({{"wR", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    processor.execute("wait 1000", sink);

    std::ostringstream first_print;
    processor.execute("print board", first_print);
    CHECK_EQ(first_print.str(), "wR . .");

    processor.execute("wait 1000", sink);

    std::ostringstream second_print;
    processor.execute("print board", second_print);
    CHECK_EQ(second_print.str(), ". . wR");
}

TEST_CASE("GameStateTest - IsPieceMovingWhileInTransit") {
    kfc::GameState state(make_board({{"wR", ".", "."}}));
    state.select(0, 0);
    state.move_selected_to(0, 2);
    CHECK(state.is_piece_moving(0, 0));
    CHECK_FALSE(state.is_piece_moving(0, 1));
    CHECK_FALSE(state.is_piece_moving(0, 2));
}

TEST_CASE("GameStateTest - IsPieceMovingFalseAfterSettleNoCooldown") {
    kfc::GameState state(make_board({{"wR", ".", "."}}));
    state.select(0, 0);
    state.move_selected_to(0, 2);
    CHECK(state.is_piece_moving(0, 0));

    state.add_clock(2000);
    CHECK_FALSE(state.is_piece_moving(0, 0));
    CHECK_FALSE(state.is_piece_moving(0, 2));
    CHECK(state.is_selectable_piece(0, 2));
}

TEST_CASE("CommandProcessorTest - ClickOnMovingPieceDoesNotSelectOrRedirect") {
    kfc::BoardModel board = make_board({{"wR", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
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
    CHECK_EQ(output.str(), ". . wR");
}

TEST_CASE("CommandProcessorTest - PieceCanMoveImmediatelyAfterSettle") {
    kfc::BoardModel board = make_board({{"wR", ".", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 150 50", sink);
    processor.execute("wait 1000", sink);
    CHECK_FALSE(state.is_piece_moving(0, 1));

    processor.execute("click 150 50", sink);
    CHECK(state.has_selection());

    processor.execute("click 250 50", sink);
    processor.execute("wait 1000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    CHECK_EQ(output.str(), ". . wR .");
}

TEST_CASE("CommandProcessorTest - OppositeColorsDoNotMoveConcurrentlyInCommonRoute") {
    kfc::BoardModel board = make_board({{"wR", ".", "."}, {".", ".", "."}, {"bR", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    processor.execute("click 50 250", sink);
    processor.execute("click 250 250", sink);
    processor.execute("wait 2000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    CHECK_EQ(output.str(), ". . wR\n. . .\nbR . .");
}

TEST_CASE("CommandProcessorTest - OppositeColorsCanMoveOnDisjointRoutes") {
    kfc::BoardModel board = make_board({{"wR", ".", ".", "."}, {".", ".", ".", "."}, {".", ".", "bR", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 150 50", sink);
    processor.execute("click 250 250", sink);
    processor.execute("click 350 250", sink);
    processor.execute("wait 1000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    CHECK_EQ(output.str(), ". wR . .\n. . . .\n. . . bR");
}

TEST_CASE("CommandProcessorTest - MovingPieceIgnoresRedirect") {
    kfc::BoardModel board = make_board({{"wR", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    processor.execute("wait 1000", sink);
    processor.execute("click 50 50", sink);
    processor.execute("click 150 50", sink);
    processor.execute("wait 1000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    CHECK_EQ(output.str(), ". . wR");
}

TEST_CASE("GameStateTest - MoveAbortedIfFriendlyOccupiesTargetBeforeArrival") {
    kfc::BoardModel board = make_board({{"wR", ".", ".", "."}, {".", ".", ".", "."}});
    kfc::GameState state(board);
    state.select(0, 0);
    state.move_selected_to(0, 2);

    state.set_piece(0, 2, *kfc::Piece::from_token("wK"));

    state.add_clock(2000);

    CHECK_EQ(state.token_at(0, 0), "wR");
    CHECK_EQ(state.token_at(0, 2), "wK");
}

TEST_CASE("CommandProcessorTest - RejectsMoveForPieceAlreadyInPendingMove") {
    kfc::BoardModel board = make_board({{"wR", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
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
    CHECK_EQ(output.str(), ". . wR");
}

TEST_CASE("CommandProcessorTest - RejectsTwoSameColorMovesToSameSquare") {
    kfc::BoardModel board = make_board({{"wR", ".", ".", "."}, {".", ".", ".", "."}, {".", ".", ".", "wN"}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    processor.execute("click 350 250", sink);
    processor.execute("click 250 50", sink);
    processor.execute("wait 2000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    CHECK_EQ(output.str(), ". . wR .\n. . . .\n. . . wN");
}

TEST_CASE("CommandProcessorTest - KingCaptureSetsGameOver") {
    kfc::BoardModel board = make_board({{"wR", ".", "bK"}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
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

TEST_CASE("CommandProcessorTest - CommandsIgnoredAfterGameOver") {
    kfc::BoardModel board = make_board({{"wR", ".", "bK"}, {".", "wN", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    processor.execute("wait 2000", sink);
    CHECK(state.is_game_over());

    const BoardLayout board_snapshot = capture_layout(state);
    const std::int64_t clock_snapshot = state.clock_ms();

    processor.execute("click 150 150", sink);
    CHECK_FALSE(state.has_selection());

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    CHECK_FALSE(state.has_selection());
    CHECK(layout_matches(state, board_snapshot));

    processor.execute("wait 5000", sink);
    CHECK_EQ(state.clock_ms(), clock_snapshot);
    CHECK(layout_matches(state, board_snapshot));
}

TEST_CASE("CommandProcessorTest - PrintBoardAfterKingCapture") {
    kfc::BoardModel board = make_board({{"wR", ".", "bK"}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    processor.execute("wait 2000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    CHECK_EQ(output.str(), ". . wR");
}

TEST_CASE("GameStateTest - JumpCaptureInterceptsArrivingEnemy") {
    kfc::BoardModel board = make_board({{".", ".", "."}, {"wR", ".", "."}, {"bR", ".", "."}});
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
    kfc::GameState state(make_board({{"wR", ".", "."}}));
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
    kfc::GameState state(make_board({{"wR", ".", "."}}));
    state.select(0, 0);
    state.jump_selected();
    CHECK(state.is_piece_jumping(0, 0));

    state.add_clock(1000);
    CHECK_FALSE(state.is_piece_jumping(0, 0));
    CHECK_EQ(state.token_at(0, 0), "wR");
}

TEST_CASE("CommandProcessorTest - JumpCommandAirbornePieceCapturesArrivingEnemy") {
    kfc::BoardModel board = make_board({{".", ".", "."}, {"wK", ".", "bR"}, {".", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("jump 50 150", sink);
    processor.execute("click 250 150", sink);
    processor.execute("click 50 150", sink);
    processor.execute("wait 1000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    CHECK_EQ(output.str(), ". . .\nwK . .\n. . .");
}

TEST_CASE("BoardValidatorTest - BoardErrorMessageRowWidthMismatch") {
    CHECK_EQ(std::string(kfc::board_error_message(kfc::BoardError::RowWidthMismatch)), kfc::kErrorRowWidthMismatch);
}

TEST_CASE("BoardValidatorTest - BoardErrorMessageOk") {
    CHECK_EQ(std::string(kfc::board_error_message(kfc::BoardError::Ok)), "");
}

TEST_CASE("BoardValidatorTest - ParseBoardEmptyLines") {
    kfc::BoardModel board;
    CHECK_EQ(kfc::parse_board_rows({}, board), kfc::BoardError::Ok);
    CHECK_EQ(board.rows(), 0);
}

TEST_CASE("BoardModelTest - BoardModelEquality") {
    const kfc::BoardModel left = make_board({{"wK", ".", "bK"}});
    const kfc::BoardModel same = make_board({{"wK", ".", "bK"}});
    const kfc::BoardModel different = make_board({{"wK", ".", "."}});
    CHECK_EQ(left, same);
    CHECK_FALSE(left == different);
}

TEST_CASE("BoardModelTest - BoardModelContainsNegative") {
    const kfc::BoardModel board = make_board({{"wK", ".", "bK"}});
    CHECK_FALSE(board.contains(-1, 0));
    CHECK_FALSE(board.contains(0, -1));
    CHECK(board.contains(0, 0));
    CHECK(board.contains(0, 2));
}

TEST_CASE("PieceTest - PieceFromTokenInvalid") {
    CHECK_FALSE(kfc::Piece::from_token("xZ").has_value());
    CHECK_FALSE(kfc::Piece::from_token("wX").has_value());
    CHECK_FALSE(kfc::Piece::from_token("").has_value());
    CHECK_FALSE(kfc::Piece::from_token("w").has_value());
    CHECK_FALSE(kfc::Piece::from_token("abc").has_value());
}

TEST_CASE("PieceTest - PieceToToken") {
    CHECK_EQ(kfc::Piece::empty().to_token(), ".");
    const std::optional<kfc::Piece> piece = kfc::Piece::from_token("bR");
    REQUIRE(piece.has_value());
    CHECK_EQ(piece->to_token(), "bR");
}

TEST_CASE("PieceTest - PieceColorHelpers") {
    const kfc::Piece white = *kfc::Piece::from_token("wN");
    const kfc::Piece black = *kfc::Piece::from_token("bN");
    const kfc::Piece empty = kfc::Piece::empty();
    CHECK(white.is_white());
    CHECK_FALSE(white.is_black());
    CHECK(black.is_black());
    CHECK(white.is_same_color_as(white));
    CHECK_FALSE(white.is_same_color_as(black));
    CHECK_FALSE(white.is_same_color_as(empty));
    CHECK(white.is_opponent_of(black));
    CHECK_FALSE(white.is_opponent_of(white));
    CHECK_NE(white, black);
    CHECK_EQ(white, white);
}

TEST_CASE("MoveValidatorTest - BishopDiagonalMove") {
    const kfc::BoardModel board = make_board({{".", ".", "."}, {".", "wB", "."}, {".", ".", "."}});
    CHECK(kfc::is_legal_move(board, 'B', 1, 1, 0, 0));
    CHECK(kfc::is_legal_move(board, 'B', 1, 1, 2, 2));
    CHECK_FALSE(kfc::is_legal_move(board, 'B', 1, 1, 1, 2));
    CHECK_FALSE(kfc::is_legal_move(board, 'B', 1, 1, 2, 1));
}

TEST_CASE("MoveValidatorTest - BishopBlockedByPiece") {
    const kfc::BoardModel board = make_board({{"wP", ".", "."}, {".", "wB", "."}, {".", ".", "."}});
    CHECK_FALSE(kfc::is_legal_move(board, 'B', 1, 1, 0, 0));
}

TEST_CASE("MoveValidatorTest - BishopCapturesEnemy") {
    const kfc::BoardModel board = make_board({{".", ".", "bP"}, {".", "wB", "."}, {".", ".", "."}});
    CHECK(kfc::is_legal_move(board, 'B', 1, 1, 0, 2));
}

TEST_CASE("MoveValidatorTest - QueenStraightAndDiagonalMoves") {
    const kfc::BoardModel board =
        make_board({{".", ".", ".", "."}, {".", "wQ", ".", "."}, {".", ".", ".", "."}});
    CHECK(kfc::is_legal_move(board, 'Q', 1, 1, 1, 3));
    CHECK(kfc::is_legal_move(board, 'Q', 1, 1, 0, 0));
    CHECK_FALSE(kfc::is_legal_move(board, 'Q', 1, 1, 0, 3));
}

TEST_CASE("MoveValidatorTest - QueenBlockedByFriendlyPiece") {
    const kfc::BoardModel board = make_board({{".", "wP", ".", "."}, {".", "wQ", ".", "."}});
    CHECK_FALSE(kfc::is_legal_move(board, 'Q', 1, 1, 0, 1));
}

TEST_CASE("PathUtilsTest - ForEachCellOnPath") {
    std::vector<std::pair<int, int>> cells;
    kfc::for_each_cell_on_path(0, 0, 0, 3, [&](int row, int col) {
        cells.emplace_back(row, col);
    });
    CHECK_EQ(cells.size(), 2);
    CHECK_EQ(cells[0], std::make_pair(0, 1));
    CHECK_EQ(cells[1], std::make_pair(0, 2));

    cells.clear();
    kfc::for_each_cell_on_path(0, 0, 2, 2, [&](int row, int col) {
        cells.emplace_back(row, col);
    });
    CHECK_EQ(cells.size(), 1);
    CHECK_EQ(cells[0], std::make_pair(1, 1));
}

TEST_CASE("PathUtilsTest - PathsShareCellOnOverlappingRoutes") {
    CHECK(kfc::paths_share_cell({0, 0}, {0, 3}, {0, 1}, {0, 4}));
    CHECK(kfc::paths_share_cell({0, 0}, {2, 2}, {0, 2}, {2, 0}));
    CHECK_FALSE(kfc::paths_share_cell({0, 0}, {0, 2}, {1, 0}, {1, 2}));
}

TEST_CASE("PathUtilsTest - PathsShareCellNonStraightPaths") {
    CHECK(kfc::paths_share_cell({0, 0}, {0, 2}, {0, 2}, {2, 2}));
    CHECK_FALSE(kfc::paths_share_cell({0, 0}, {0, 1}, {2, 0}, {2, 1}));
}

TEST_CASE("CommandProcessorTest - CommandProcessorWaitWithoutMs") {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    CHECK_EQ(state.clock_ms(), 0);
    processor.execute("wait", sink);
    CHECK_EQ(state.clock_ms(), 0);
}

TEST_CASE("CommandProcessorTest - CommandProcessorUnknownVerb") {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("foobar 1 2", sink);
    CHECK_FALSE(state.has_selection());
    CHECK_EQ(state.clock_ms(), 0);
}

TEST_CASE("GameStateTest - GameStateCustomRules") {
    kfc::GameRules rules;
    rules.is_legal_move = kfc::is_legal_move;
    rules.on_reach_last_row = [](kfc::Piece piece, std::size_t, std::size_t) { return piece; };
    rules.is_game_over = [](kfc::Piece) { return false; };
    rules.move_duration_ms = 500;
    rules.jump_duration_ms = 300;

    kfc::BoardModel board = make_board({{"wR", ".", "."}});
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

TEST_CASE("CommandProcessorTest - JumpCommandTooLateDoesNotSavePiece") {
    kfc::BoardModel board = make_board({{".", ".", "."}, {"wK", ".", "bR"}, {".", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 250 150", sink);
    processor.execute("click 50 150", sink);
    processor.execute("wait 1000", sink);
    processor.execute("jump 50 150", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    CHECK_EQ(output.str(), ". . .\nbR . .\n. . .");
}

TEST_CASE("BoardValidatorTest - IsValidTokenInvalidColor") {
    CHECK_FALSE(kfc::is_valid_token("xK"));
    CHECK_FALSE(kfc::is_valid_token("zP"));
}

TEST_CASE("PieceTest - PieceOpponentOfEmpty") {
    const kfc::Piece white = *kfc::Piece::from_token("wK");
    const kfc::Piece empty = kfc::Piece::empty();
    CHECK_FALSE(white.is_opponent_of(empty));
    CHECK_FALSE(empty.is_opponent_of(white));
}

TEST_CASE("MoveValidatorTest - IllegalMoveEmptyBoard") {
    const kfc::BoardModel board;
    CHECK_FALSE(kfc::is_legal_move(board, 'K', 0, 0, 0, 1));
}

TEST_CASE("MoveValidatorTest - IllegalMoveSameCell") {
    const kfc::BoardModel board = make_board({{"wK", ".", "bK"}});
    CHECK_FALSE(kfc::is_legal_move(board, 'K', 0, 0, 0, 0));
}

TEST_CASE("MoveValidatorTest - IllegalMoveUnknownPieceType") {
    const kfc::BoardModel board = make_board({{"wK", ".", "bK"}});
    CHECK_FALSE(kfc::is_legal_move(board, 'X', 0, 0, 0, 1));
}

TEST_CASE("BoardValidatorTest - ParseBoardEmptyRowTokens") {
    const std::vector<std::string> lines = {""};
    kfc::BoardModel board;
    CHECK_EQ(kfc::parse_board_rows(lines, board), kfc::BoardError::RowWidthMismatch);
}

TEST_CASE("BoardModelTest - BoardModelInBoundsEmptyBoard") {
    const kfc::BoardModel board;
    CHECK_FALSE(board.is_in_bounds(0, 0));
    CHECK_FALSE(board.contains(0, 0));
    CHECK_FALSE(board.contains(5, 5));
}

TEST_CASE("BoardModelTest - BoardModelContainsOutOfBounds") {
    const kfc::BoardModel board = make_board({{"wK", ".", "bK"}});
    CHECK_FALSE(board.contains(0, 3));
    CHECK_FALSE(board.contains(1, 0));
}

TEST_CASE("BoardModelTest - BoardModelEqualityDifferentSizes") {
    const kfc::BoardModel narrow = make_board({{"wK"}});
    const kfc::BoardModel wide = make_board({{"wK", "."}});
    CHECK_FALSE(narrow == wide);
}

TEST_CASE("BoardWriterTest - WriteEmptyBoard") {
    std::ostringstream output;
    kfc::write_board(output, kfc::BoardModel{});
    CHECK(output.str().empty());
}

TEST_CASE("BoardWriterTest - WriteSingleRowBoard") {
    std::ostringstream output;
    kfc::write_board(output, make_board({{"wK", "bK"}}));
    CHECK_EQ(output.str(), "wK bK");
}

TEST_CASE("PathUtilsTest - ForEachCellOnPathAdjacent") {
    std::vector<std::pair<int, int>> cells;
    kfc::for_each_cell_on_path(0, 0, 0, 1, [&](int row, int col) {
        cells.emplace_back(row, col);
    });
    CHECK(cells.empty());
}

TEST_CASE("PathUtilsTest - ForEachCellOnPathVertical") {
    std::vector<std::pair<int, int>> cells;
    kfc::for_each_cell_on_path(0, 0, 3, 0, [&](int row, int col) {
        cells.emplace_back(row, col);
    });
    CHECK_EQ(cells.size(), 2);
    CHECK_EQ(cells[0], std::make_pair(1, 0));
    CHECK_EQ(cells[1], std::make_pair(2, 0));
}

TEST_CASE("PathUtilsTest - PathsShareCellEndpointOverlap") {
    CHECK(kfc::paths_share_cell({0, 0}, {0, 1}, {0, 1}, {0, 2}));
}

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
    kfc::BoardModel board = make_board({{"wR", ".", "bP"}});
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
    kfc::BoardModel board = make_board({{"wR", ".", "."}});
    kfc::CollisionResolver resolver;
    const kfc::GameRules rules = kfc::KungFuChessRules::standard();
    bool game_over = false;
    const kfc::ArrivingPieceInfo arriving{
        *kfc::Piece::from_token("wR"), {0, 0}, {0, 2}};
    CHECK_FALSE(resolver.check_for_jump_capture(board, rules, 1000, {}, {0, 2}, arriving, game_over));
    CHECK_EQ(board.token_at(0, 0), "wR");
}

TEST_CASE("CollisionResolverTest - CollisionJumpCaptureSetsGameOver") {
    kfc::BoardModel board = make_board({{".", ".", "."}, {"wK", ".", "."}, {".", ".", "."}});
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

TEST_CASE("VplIoTest - ReadVplMultipleCommandsAndSkipBlankLines") {
    const std::string input =
        "Board:\n"
        "wK . bK\n"
        "Commands:\n"
        "wait 100\n"
        "\n"
        "print board\n";

    std::istringstream in(input);
    const kfc::VplInput parsed = kfc::read_vpl_input(in);
    CHECK_EQ(parsed.error, kfc::BoardError::Ok);
    CHECK_EQ(parsed.commands.size(), 2);
    CHECK_EQ(parsed.commands[0], "wait 100");
    CHECK_EQ(parsed.commands[1], "print board");
}

TEST_CASE("VplIoTest - ReadVplNoSectionHeaders") {
    const std::string input = "wK . bK\n";
    std::istringstream in(input);
    const kfc::VplInput parsed = kfc::read_vpl_input(in);
    CHECK_EQ(parsed.error, kfc::BoardError::Ok);
    CHECK_EQ(parsed.board.rows(), 0);
    CHECK(parsed.commands.empty());
}

TEST_CASE("VplIoTest - ReadVplWhitespacePaddedBoardLine") {
    const std::string input =
        "Board:\n"
        "  wK . bK  \n"
        "Commands:\n";

    std::istringstream in(input);
    const kfc::VplInput parsed = kfc::read_vpl_input(in);
    CHECK_EQ(parsed.error, kfc::BoardError::Ok);
    CHECK_EQ(parsed.board.token_at(0, 0), "wK");
    CHECK_EQ(parsed.board.token_at(0, 2), "bK");
}

TEST_CASE("CommandProcessorTest - CommandProcessorClickWithoutCoords") {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click", sink);
    CHECK_FALSE(state.has_selection());
}

TEST_CASE("CommandProcessorTest - CommandProcessorJumpWithoutCoords") {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("jump", sink);
    CHECK_FALSE(state.is_piece_jumping(0, 0));
}

TEST_CASE("CommandProcessorTest - CommandProcessorPrintPartialCommandIgnored") {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    kfc::CommandProcessor processor(state);
    std::ostringstream output;

    processor.execute("print", output);
    CHECK(output.str().empty());
    processor.execute("print foo", output);
    CHECK(output.str().empty());
}

TEST_CASE("CommandProcessorTest - CommandProcessorSelectEmptySquare") {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 150 50", sink);
    CHECK_FALSE(state.has_selection());
}

TEST_CASE("CommandProcessorTest - CommandProcessorFriendlyClickSameCellJumps") {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    CHECK(state.has_selection());
    processor.execute("click 50 50", sink);
    CHECK_FALSE(state.has_selection());
    CHECK(state.is_piece_jumping(0, 0));
}

TEST_CASE("CommandProcessorTest - CommandProcessorJumpOutsideGrid") {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("jump 350 50", sink);
    CHECK_FALSE(state.is_piece_jumping(0, 0));
}

TEST_CASE("CommandProcessorTest - CommandProcessorJumpOnEmptyCell") {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("jump 150 50", sink);
    CHECK_FALSE(state.is_piece_jumping(0, 1));
}

TEST_CASE("GameStateTest - GameStateSelectOutOfBounds") {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    state.select(99, 99);
    CHECK_FALSE(state.has_selection());
}

TEST_CASE("GameStateTest - GameStateMoveAndJumpWithoutSelection") {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    state.move_selected_to(0, 1);
    state.jump_selected();
    CHECK_EQ(state.token_at(0, 0), "wK");
    CHECK_FALSE(state.is_piece_jumping(0, 0));
}

TEST_CASE("GameStateTest - GameStateJumpAtEmptyCell") {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    state.jump_at(0, 1);
    CHECK_FALSE(state.is_piece_jumping(0, 1));
}

TEST_CASE("GameStateTest - GameStateIsPieceOutOfBounds") {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    CHECK_FALSE(state.is_piece(99, 99));
}

TEST_CASE("GameStateTest - GameStateFriendlySelectionRequiresSelection") {
    kfc::GameState state(make_board({{"wK", "wN", "bK"}}));
    CHECK_FALSE(state.is_friendly_to_selection(0, 1));
}

TEST_CASE("GameStateTest - GameStateSameBoardLayoutAs") {
    const kfc::GameState left(make_board({{"wK", ".", "bK"}}));
    const kfc::GameState right(make_board({{"wK", ".", "bK"}}));
    const kfc::GameState different(make_board({{"wK", ".", "."}}));
    CHECK(left.same_board_layout_as(right));
    CHECK_FALSE(left.same_board_layout_as(different));
}

TEST_CASE("GameStateTest - GameStateCaptureMoveUsesSingleCellDuration") {
    kfc::GameState state(make_board({{"wR", ".", "bP"}}));
    state.select(0, 0);
    state.move_selected_to(0, 2);
    state.add_clock(999);
    CHECK_EQ(state.token_at(0, 0), "wR");
    CHECK_EQ(state.token_at(0, 2), "bP");
    state.add_clock(1);
    CHECK_EQ(state.token_at(0, 0), ".");
    CHECK_EQ(state.token_at(0, 2), "wR");
}

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

TEST_CASE("BoardModelTest - ValidRectangularBoard") {
    const kfc::BoardModel board =
        make_board({{"wK", ".", "bK"}, {".", "wN", "."}, {"bP", ".", "wR"}});
    CHECK(board.is_valid());
}

}  // namespace

