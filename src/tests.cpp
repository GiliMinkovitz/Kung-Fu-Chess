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

#include <gtest/gtest.h>
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

TEST(BoardModelTest, InvalidEmptyBoard) {
    const kfc::BoardModel board;
    EXPECT_FALSE(board.is_valid());
}

TEST(BoardModelTest, InvalidNonRectangularBoard) {
    const kfc::BoardModel board = make_board({{"wK", "."}, {"bK"}});
    EXPECT_FALSE(board.is_valid());
}

TEST(BoardValidatorTest, InvalidUnknownToken) {
    EXPECT_FALSE(kfc::is_valid_token("xZ"));
    EXPECT_TRUE(kfc::is_valid_token("."));
    EXPECT_TRUE(kfc::is_valid_token("wK"));
    EXPECT_TRUE(kfc::is_valid_token("bQ"));
}

TEST(BoardValidatorTest, ParseBoardRowsSuccess) {
    const std::vector<std::string> lines = {"wK . . bK", ". . . .", "wR . . bR"};
    kfc::BoardModel board;
    EXPECT_EQ(kfc::parse_board_rows(lines, board), kfc::BoardError::Ok);
    EXPECT_EQ(board.rows(), 3);
    EXPECT_EQ(board.cols(), 4);
    EXPECT_EQ(board.token_at(0, 0), "wK");
    EXPECT_EQ(board.token_at(2, 3), "bR");
}

TEST(BoardValidatorTest, ParseBoardUnknownToken) {
    const std::vector<std::string> lines = {"wK xZ", ". ."};
    kfc::BoardModel board;
    EXPECT_EQ(kfc::parse_board_rows(lines, board), kfc::BoardError::UnknownToken);
}

TEST(BoardValidatorTest, ParseBoardRowWidthMismatch) {
    const std::vector<std::string> lines = {"wK . .", ". bK"};
    kfc::BoardModel board;
    EXPECT_EQ(kfc::parse_board_rows(lines, board), kfc::BoardError::RowWidthMismatch);
}

TEST(VplIoTest, ReadAndWriteRoundtrip) {
    const std::string input =
        "Board:\n"
        "wK . bQ\n"
        ". wN .\n"
        "bP . wR\n"
        "Commands:\n"
        "print board\n";

    std::istringstream in(input);
    const kfc::VplInput parsed = kfc::read_vpl_input(in);
    EXPECT_EQ(parsed.error, kfc::BoardError::Ok);
    EXPECT_TRUE(parsed.board.is_valid());
    EXPECT_EQ(parsed.commands.size(), 1);
    EXPECT_EQ(parsed.commands[0], "print board");

    std::ostringstream output;
    kfc::write_board(output, parsed.board);
    EXPECT_EQ(output.str(), "wK . bQ\n. wN .\nbP . wR");
}

TEST(VplIoTest, InvalidBoardProducesError) {
    const std::string input =
        "Board:\n"
        "wK xZ\n"
        ". .\n"
        "Commands:\n";

    std::istringstream in(input);
    const kfc::VplInput parsed = kfc::read_vpl_input(in);
    EXPECT_EQ(parsed.error, kfc::BoardError::UnknownToken);
    EXPECT_EQ(std::string(kfc::board_error_message(parsed.error)), kfc::kErrorUnknownToken);
}

TEST(GameStateTest, GameStateWaitIncrementsClock) {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    EXPECT_EQ(state.clock_ms(), 0);
    state.add_clock(250);
    state.add_clock(50);
    EXPECT_EQ(state.clock_ms(), 300);
}

TEST(CommandProcessorTest, CommandProcessorClickSelectAndMove) {
    kfc::BoardModel board = make_board({{"wK", ".", "bK"}, {".", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    EXPECT_TRUE(state.has_selection());
    std::size_t row = 0;
    std::size_t col = 0;
    ASSERT_TRUE(state.selection(row, col));
    EXPECT_EQ(row, 0u);
    EXPECT_EQ(col, 0u);

    processor.execute("click 150 150", sink);
    EXPECT_FALSE(state.has_selection());
    EXPECT_EQ(state.token_at(0, 0), "wK");
    EXPECT_EQ(state.token_at(1, 1), ".");

    processor.execute("wait 1000", sink);
    EXPECT_EQ(state.token_at(0, 0), ".");
    EXPECT_EQ(state.token_at(1, 1), "wK");
}

TEST(CommandProcessorTest, CommandProcessorClickOutsideGridIgnored) {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 350 50", sink);
    processor.execute("click -10 50", sink);
    EXPECT_FALSE(state.has_selection());
}

TEST(CommandProcessorTest, CommandProcessorFriendlyClickReplacesSelection) {
    kfc::BoardModel board = make_board({{"wK", "wN", "bK"}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 150 50", sink);
    EXPECT_TRUE(state.has_selection());
    std::size_t row = 0;
    std::size_t col = 0;
    ASSERT_TRUE(state.selection(row, col));
    EXPECT_EQ(row, 0u);
    EXPECT_EQ(col, 1u);
    EXPECT_EQ(state.token_at(0, 0), "wK");
    EXPECT_EQ(state.token_at(0, 1), "wN");
}

TEST(CommandProcessorTest, CommandProcessorPrintBoard) {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    kfc::CommandProcessor processor(state);

    std::ostringstream output;
    processor.execute("print board", output);
    EXPECT_EQ(output.str(), "wK . bK");
}

TEST(MoveValidatorTest, KnightLegalLMove) {
    const kfc::BoardModel board = make_board({{".", ".", ".", ".", "."},
                              {".", ".", ".", ".", "."},
                              {".", ".", "wN", ".", "."},
                              {".", ".", ".", ".", "."},
                              {".", ".", ".", ".", "."}});
    EXPECT_TRUE(kfc::is_legal_move(board, 'N', 2, 2, 4, 3));
    EXPECT_TRUE(kfc::is_legal_move(board, 'N', 2, 2, 0, 3));
    EXPECT_FALSE(kfc::is_legal_move(board, 'N', 2, 2, 2, 4));
}

TEST(MoveValidatorTest, RookCannotMoveDiagonally) {
    const kfc::BoardModel board = make_board({{".", ".", "."}, {".", "wR", "."}, {".", ".", "."}});
    EXPECT_FALSE(kfc::is_legal_move(board, 'R', 1, 1, 2, 2));
    EXPECT_FALSE(kfc::is_legal_move(board, 'R', 1, 1, 0, 0));
    EXPECT_TRUE(kfc::is_legal_move(board, 'R', 1, 1, 1, 0));
    EXPECT_TRUE(kfc::is_legal_move(board, 'R', 1, 1, 2, 1));
}

TEST(MoveValidatorTest, KingCannotMoveMoreThanOneSquare) {
    const kfc::BoardModel board = make_board({{".", ".", ".", "."}, {".", "wK", ".", "."}, {".", ".", ".", "."}});
    EXPECT_FALSE(kfc::is_legal_move(board, 'K', 1, 1, 1, 3));
    EXPECT_FALSE(kfc::is_legal_move(board, 'K', 1, 1, 3, 1));
    EXPECT_FALSE(kfc::is_legal_move(board, 'K', 1, 1, 3, 3));
    EXPECT_TRUE(kfc::is_legal_move(board, 'K', 1, 1, 1, 2));
    EXPECT_TRUE(kfc::is_legal_move(board, 'K', 1, 1, 2, 2));
}

TEST(MoveValidatorTest, MoveRespectsBoardBoundaries) {
    const kfc::BoardModel board = make_board({{"wN", ".", "wR"}, {".", "wK", "."}});
    EXPECT_FALSE(kfc::is_legal_move(board, 'N', 0, 0, -1, 1));
    EXPECT_FALSE(kfc::is_legal_move(board, 'R', 0, 2, 0, 5));
    EXPECT_FALSE(kfc::is_legal_move(board, 'K', 1, 1, 2, 3));
}

TEST(MoveValidatorTest, RookBlockedByFriendlyPiece) {
    const kfc::BoardModel board = make_board({{".", ".", ".", "."}, {".", "wR", "wP", "."}});
    EXPECT_FALSE(kfc::is_legal_move(board, 'R', 1, 1, 1, 3));
    EXPECT_TRUE(kfc::is_legal_move(board, 'R', 1, 1, 1, 0));
}

TEST(MoveValidatorTest, RookCapturesEnemyPiece) {
    const kfc::BoardModel board = make_board({{".", ".", ".", "."}, {".", "wR", ".", "bP"}});
    EXPECT_TRUE(kfc::is_legal_move(board, 'R', 1, 1, 1, 3));
}

TEST(MoveValidatorTest, KnightJumpsOverPieces) {
    const kfc::BoardModel board = make_board({{".", ".", ".", ".", "."},
                              {".", "wP", "bN", ".", "."},
                              {".", ".", "wN", ".", "."},
                              {".", ".", ".", ".", "."},
                              {".", ".", ".", ".", "."}});
    EXPECT_TRUE(kfc::is_legal_move(board, 'N', 2, 2, 0, 3));
    EXPECT_TRUE(kfc::is_legal_move(board, 'N', 2, 2, 4, 1));
}

TEST(MoveValidatorTest, CannotCaptureOwnPiece) {
    const kfc::BoardModel board = make_board({{".", "wP", "wR"}});
    EXPECT_FALSE(kfc::is_legal_move(board, 'R', 0, 2, 0, 1));
    EXPECT_FALSE(kfc::is_legal_move(board, 'N', 0, 2, 0, 1));
}

TEST(CommandProcessorTest, CommandProcessorCapture) {
    kfc::BoardModel board = make_board({{"wR", ".", "bK"}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    EXPECT_TRUE(state.has_selection());

    processor.execute("click 250 50", sink);
    EXPECT_FALSE(state.has_selection());
    EXPECT_EQ(state.token_at(0, 0), "wR");
    EXPECT_EQ(state.token_at(0, 2), "bK");

    processor.execute("wait 2000", sink);
    EXPECT_EQ(state.token_at(0, 0), ".");
    EXPECT_EQ(state.token_at(0, 2), "wR");
}

TEST(MoveValidatorTest, WhitePawnForwardMove) {
    const kfc::BoardModel board = make_board({{".", ".", "."}, {".", "wP", "."}, {".", ".", "."}});
    EXPECT_TRUE(kfc::is_legal_move(board, 'P', 1, 1, 0, 1));
    EXPECT_FALSE(kfc::is_legal_move(board, 'P', 1, 1, 0, 0));
    EXPECT_FALSE(kfc::is_legal_move(board, 'P', 1, 1, 0, 2));
}

TEST(MoveValidatorTest, WhitePawnBlockedForward) {
    const kfc::BoardModel board = make_board({{".", "bK", "."}, {".", "wP", "."}});
    EXPECT_FALSE(kfc::is_legal_move(board, 'P', 1, 1, 0, 1));
}

TEST(MoveValidatorTest, WhitePawnDiagonalCapture) {
    const kfc::BoardModel board = make_board({{"bN", ".", "bR"}, {".", "wP", "."}, {".", ".", "."}});
    EXPECT_TRUE(kfc::is_legal_move(board, 'P', 1, 1, 0, 0));
    EXPECT_TRUE(kfc::is_legal_move(board, 'P', 1, 1, 0, 2));

    const kfc::BoardModel empty_diagonal = make_board({{".", ".", "."}, {".", "wP", "."}});
    EXPECT_FALSE(kfc::is_legal_move(empty_diagonal, 'P', 1, 1, 0, 0));
    EXPECT_FALSE(kfc::is_legal_move(empty_diagonal, 'P', 1, 1, 0, 2));
}

TEST(MoveValidatorTest, WhitePawnCannotMoveBackwardOrForwardCapture) {
    const kfc::BoardModel board = make_board({{".", "wP", "."}, {".", "bK", "."}, {".", ".", "."}});
    EXPECT_FALSE(kfc::is_legal_move(board, 'P', 0, 1, 1, 1));
    EXPECT_FALSE(kfc::is_legal_move(board, 'P', 0, 1, 1, 0));
    EXPECT_FALSE(kfc::is_legal_move(board, 'P', 0, 1, 1, 2));
}

TEST(MoveValidatorTest, WhitePawnCannotCaptureOwnPiece) {
    const kfc::BoardModel board = make_board({{"wN", ".", "wR"}, {".", "wP", "."}});
    EXPECT_FALSE(kfc::is_legal_move(board, 'P', 1, 1, 0, 0));
    EXPECT_FALSE(kfc::is_legal_move(board, 'P', 1, 1, 0, 2));
}

TEST(MoveValidatorTest, BlackPawnForwardMove) {
    const kfc::BoardModel board = make_board({{".", ".", "."}, {".", "bP", "."}, {".", ".", "."}});
    EXPECT_TRUE(kfc::is_legal_move(board, 'P', 1, 1, 2, 1));
    EXPECT_FALSE(kfc::is_legal_move(board, 'P', 1, 1, 2, 0));
    EXPECT_FALSE(kfc::is_legal_move(board, 'P', 1, 1, 2, 2));
}

TEST(MoveValidatorTest, BlackPawnBlockedForward) {
    const kfc::BoardModel board = make_board({{".", "bP", "."}, {".", "wK", "."}});
    EXPECT_FALSE(kfc::is_legal_move(board, 'P', 0, 1, 1, 1));
}

TEST(MoveValidatorTest, BlackPawnDiagonalCapture) {
    const kfc::BoardModel board = make_board({{".", ".", "."}, {".", "bP", "."}, {"wR", ".", "wN"}});
    EXPECT_TRUE(kfc::is_legal_move(board, 'P', 1, 1, 2, 0));
    EXPECT_TRUE(kfc::is_legal_move(board, 'P', 1, 1, 2, 2));

    const kfc::BoardModel empty_diagonal = make_board({{".", "bP", "."}, {".", ".", "."}});
    EXPECT_FALSE(kfc::is_legal_move(empty_diagonal, 'P', 0, 1, 1, 0));
    EXPECT_FALSE(kfc::is_legal_move(empty_diagonal, 'P', 0, 1, 1, 2));
}

TEST(MoveValidatorTest, BlackPawnCannotMoveBackwardOrForwardCapture) {
    const kfc::BoardModel board = make_board({{".", ".", "."}, {".", "wK", "."}, {".", "bP", "."}});
    EXPECT_FALSE(kfc::is_legal_move(board, 'P', 2, 1, 1, 1));
    EXPECT_FALSE(kfc::is_legal_move(board, 'P', 2, 1, 1, 0));
    EXPECT_FALSE(kfc::is_legal_move(board, 'P', 2, 1, 1, 2));
}

TEST(MoveValidatorTest, PawnDoubleMoveFromStartRow) {
    const kfc::BoardModel white_board = make_board({{".", ".", "."},
                                    {".", ".", "."},
                                    {".", ".", "."},
                                    {".", "wP", "."}});
    EXPECT_TRUE(kfc::is_legal_move(white_board, 'P', 3, 1, 1, 1));
    EXPECT_FALSE(kfc::is_legal_move(white_board, 'P', 2, 1, 0, 1));

    const kfc::BoardModel black_board = make_board({{".", "bP", "."},
                                    {".", ".", "."},
                                    {".", ".", "."},
                                    {".", ".", "."}});
    EXPECT_TRUE(kfc::is_legal_move(black_board, 'P', 0, 1, 2, 1));
    EXPECT_FALSE(kfc::is_legal_move(black_board, 'P', 1, 1, 3, 1));
}

TEST(MoveValidatorTest, PawnDoubleMoveBlockedByIntermediatePiece) {
    const kfc::BoardModel blocked_intermediate = make_board({{".", "bP", "."},
                                             {".", "wN", "."},
                                             {".", ".", "."},
                                             {".", ".", "."}});
    EXPECT_FALSE(kfc::is_legal_move(blocked_intermediate, 'P', 0, 1, 2, 1));

    const kfc::BoardModel blocked_dest = make_board({{".", "bP", "."},
                                     {".", ".", "."},
                                     {".", "wR", "."},
                                     {".", ".", "."}});
    EXPECT_FALSE(kfc::is_legal_move(blocked_dest, 'P', 0, 1, 2, 1));
}

TEST(GameStateTest, PawnPromotionToQueen) {
    kfc::BoardModel white_board = make_board({{".", ".", "."}, {".", "wP", "."}});
    kfc::GameState white_state(white_board);
    white_state.select(1, 1);
    white_state.move_selected_to(0, 1);
    white_state.add_clock(1000);
    EXPECT_EQ(white_state.token_at(0, 1), "wQ");
    EXPECT_EQ(white_state.token_at(1, 1), ".");

    kfc::BoardModel black_board = make_board({{".", "bP", "."}, {".", ".", "."}});
    kfc::GameState black_state(black_board);
    black_state.select(0, 1);
    black_state.move_selected_to(1, 1);
    black_state.add_clock(1000);
    EXPECT_EQ(black_state.token_at(1, 1), "bQ");
    EXPECT_EQ(black_state.token_at(0, 1), ".");
}

TEST(CommandProcessorTest, CommandProcessorRejectsIllegalMove) {
    kfc::BoardModel board = make_board({{"wK", ".", ".", "."}, {".", ".", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    EXPECT_TRUE(state.has_selection());

    processor.execute("click 350 50", sink);
    EXPECT_TRUE(state.has_selection());
    EXPECT_EQ(state.token_at(0, 0), "wK");
    EXPECT_EQ(state.token_at(0, 3), ".");
}

TEST(CommandProcessorTest, PendingMovePrintBeforeArrival) {
    kfc::BoardModel board = make_board({{"wK", ".", "bK"}, {".", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 150 150", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    EXPECT_EQ(output.str(), "wK . bK\n. . .");
}

TEST(CommandProcessorTest, PendingMovePrintAfterArrival) {
    kfc::BoardModel board = make_board({{"wK", ".", "bK"}, {".", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 150 150", sink);
    processor.execute("wait 1000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    EXPECT_EQ(output.str(), ". . bK\n. wK .");
}

TEST(CommandProcessorTest, TwoCellMoveBeforeAndAfterArrival) {
    kfc::BoardModel board = make_board({{"wR", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    processor.execute("wait 1000", sink);

    std::ostringstream first_print;
    processor.execute("print board", first_print);
    EXPECT_EQ(first_print.str(), "wR . .");

    processor.execute("wait 1000", sink);

    std::ostringstream second_print;
    processor.execute("print board", second_print);
    EXPECT_EQ(second_print.str(), ". . wR");
}

TEST(GameStateTest, IsPieceMovingWhileInTransit) {
    kfc::GameState state(make_board({{"wR", ".", "."}}));
    state.select(0, 0);
    state.move_selected_to(0, 2);
    EXPECT_TRUE(state.is_piece_moving(0, 0));
    EXPECT_FALSE(state.is_piece_moving(0, 1));
    EXPECT_FALSE(state.is_piece_moving(0, 2));
}

TEST(GameStateTest, IsPieceMovingFalseAfterSettleNoCooldown) {
    kfc::GameState state(make_board({{"wR", ".", "."}}));
    state.select(0, 0);
    state.move_selected_to(0, 2);
    EXPECT_TRUE(state.is_piece_moving(0, 0));

    state.add_clock(2000);
    EXPECT_FALSE(state.is_piece_moving(0, 0));
    EXPECT_FALSE(state.is_piece_moving(0, 2));
    EXPECT_TRUE(state.is_selectable_piece(0, 2));
}

TEST(CommandProcessorTest, ClickOnMovingPieceDoesNotSelectOrRedirect) {
    kfc::BoardModel board = make_board({{"wR", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    processor.execute("wait 1000", sink);

    processor.execute("click 50 50", sink);
    EXPECT_FALSE(state.has_selection());

    processor.execute("click 150 50", sink);
    processor.execute("wait 1000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    EXPECT_EQ(output.str(), ". . wR");
}

TEST(CommandProcessorTest, PieceCanMoveImmediatelyAfterSettle) {
    kfc::BoardModel board = make_board({{"wR", ".", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 150 50", sink);
    processor.execute("wait 1000", sink);
    EXPECT_FALSE(state.is_piece_moving(0, 1));

    processor.execute("click 150 50", sink);
    EXPECT_TRUE(state.has_selection());

    processor.execute("click 250 50", sink);
    processor.execute("wait 1000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    EXPECT_EQ(output.str(), ". . wR .");
}

TEST(CommandProcessorTest, OppositeColorsDoNotMoveConcurrentlyInCommonRoute) {
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
    EXPECT_EQ(output.str(), ". . wR\n. . .\nbR . .");
}

TEST(CommandProcessorTest, OppositeColorsCanMoveOnDisjointRoutes) {
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
    EXPECT_EQ(output.str(), ". wR . .\n. . . .\n. . . bR");
}

TEST(CommandProcessorTest, MovingPieceIgnoresRedirect) {
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
    EXPECT_EQ(output.str(), ". . wR");
}

TEST(GameStateTest, MoveAbortedIfFriendlyOccupiesTargetBeforeArrival) {
    kfc::BoardModel board = make_board({{"wR", ".", ".", "."}, {".", ".", ".", "."}});
    kfc::GameState state(board);
    state.select(0, 0);
    state.move_selected_to(0, 2);

    state.set_piece(0, 2, *kfc::Piece::from_token("wK"));

    state.add_clock(2000);

    EXPECT_EQ(state.token_at(0, 0), "wR");
    EXPECT_EQ(state.token_at(0, 2), "wK");
}

TEST(CommandProcessorTest, RejectsMoveForPieceAlreadyInPendingMove) {
    kfc::BoardModel board = make_board({{"wR", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    EXPECT_TRUE(state.is_piece_moving(0, 0));

    processor.execute("click 50 50", sink);
    EXPECT_FALSE(state.has_selection());

    processor.execute("click 150 50", sink);
    EXPECT_FALSE(state.has_selection());

    processor.execute("wait 1000", sink);
    EXPECT_TRUE(state.is_piece_moving(0, 0));

    processor.execute("wait 1000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    EXPECT_EQ(output.str(), ". . wR");
}

TEST(CommandProcessorTest, RejectsTwoSameColorMovesToSameSquare) {
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
    EXPECT_EQ(output.str(), ". . wR .\n. . . .\n. . . wN");
}

TEST(CommandProcessorTest, KingCaptureSetsGameOver) {
    kfc::BoardModel board = make_board({{"wR", ".", "bK"}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    EXPECT_FALSE(state.is_game_over());

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    EXPECT_FALSE(state.is_game_over());

    processor.execute("wait 2000", sink);
    EXPECT_TRUE(state.is_game_over());
    EXPECT_EQ(state.token_at(0, 0), ".");
    EXPECT_EQ(state.token_at(0, 2), "wR");
}

TEST(CommandProcessorTest, CommandsIgnoredAfterGameOver) {
    kfc::BoardModel board = make_board({{"wR", ".", "bK"}, {".", "wN", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    processor.execute("wait 2000", sink);
    EXPECT_TRUE(state.is_game_over());

    const BoardLayout board_snapshot = capture_layout(state);
    const std::int64_t clock_snapshot = state.clock_ms();

    processor.execute("click 150 150", sink);
    EXPECT_FALSE(state.has_selection());

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    EXPECT_FALSE(state.has_selection());
    EXPECT_TRUE(layout_matches(state, board_snapshot));

    processor.execute("wait 5000", sink);
    EXPECT_EQ(state.clock_ms(), clock_snapshot);
    EXPECT_TRUE(layout_matches(state, board_snapshot));
}

TEST(CommandProcessorTest, PrintBoardAfterKingCapture) {
    kfc::BoardModel board = make_board({{"wR", ".", "bK"}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    processor.execute("wait 2000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    EXPECT_EQ(output.str(), ". . wR");
}

TEST(GameStateTest, JumpCaptureInterceptsArrivingEnemy) {
    kfc::BoardModel board = make_board({{".", ".", "."}, {"wR", ".", "."}, {"bR", ".", "."}});
    kfc::GameState state(board);

    state.select(2, 0);
    state.move_selected_to(1, 0);
    state.add_clock(500);

    state.select(1, 0);
    state.jump_selected();
    EXPECT_TRUE(state.is_piece_jumping(1, 0));

    state.add_clock(500);

    EXPECT_EQ(state.token_at(2, 0), ".");
    EXPECT_EQ(state.token_at(1, 0), "wR");
}

TEST(GameStateTest, MovingPieceCannotJump) {
    kfc::GameState state(make_board({{"wR", ".", "."}}));
    state.select(0, 0);
    state.move_selected_to(0, 2);
    EXPECT_TRUE(state.is_piece_moving(0, 0));

    state.select(0, 0);
    state.jump_selected();
    EXPECT_FALSE(state.is_piece_jumping(0, 0));

    state.add_clock(2000);
    EXPECT_EQ(state.token_at(0, 2), "wR");
}

TEST(GameStateTest, JumpStatusClearedAfterDuration) {
    kfc::GameState state(make_board({{"wR", ".", "."}}));
    state.select(0, 0);
    state.jump_selected();
    EXPECT_TRUE(state.is_piece_jumping(0, 0));

    state.add_clock(1000);
    EXPECT_FALSE(state.is_piece_jumping(0, 0));
    EXPECT_EQ(state.token_at(0, 0), "wR");
}

TEST(CommandProcessorTest, JumpCommandAirbornePieceCapturesArrivingEnemy) {
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
    EXPECT_EQ(output.str(), ". . .\nwK . .\n. . .");
}

TEST(BoardValidatorTest, BoardErrorMessageRowWidthMismatch) {
    EXPECT_EQ(std::string(kfc::board_error_message(kfc::BoardError::RowWidthMismatch)), kfc::kErrorRowWidthMismatch);
}

TEST(BoardValidatorTest, BoardErrorMessageOk) {
    EXPECT_EQ(std::string(kfc::board_error_message(kfc::BoardError::Ok)), "");
}

TEST(BoardValidatorTest, ParseBoardEmptyLines) {
    kfc::BoardModel board;
    EXPECT_EQ(kfc::parse_board_rows({}, board), kfc::BoardError::Ok);
    EXPECT_EQ(board.rows(), 0);
}

TEST(BoardModelTest, BoardModelEquality) {
    const kfc::BoardModel left = make_board({{"wK", ".", "bK"}});
    const kfc::BoardModel same = make_board({{"wK", ".", "bK"}});
    const kfc::BoardModel different = make_board({{"wK", ".", "."}});
    EXPECT_EQ(left, same);
    EXPECT_FALSE(left == different);
}

TEST(BoardModelTest, BoardModelContainsNegative) {
    const kfc::BoardModel board = make_board({{"wK", ".", "bK"}});
    EXPECT_FALSE(board.contains(-1, 0));
    EXPECT_FALSE(board.contains(0, -1));
    EXPECT_TRUE(board.contains(0, 0));
    EXPECT_TRUE(board.contains(0, 2));
}

TEST(PieceTest, PieceFromTokenInvalid) {
    EXPECT_FALSE(kfc::Piece::from_token("xZ").has_value());
    EXPECT_FALSE(kfc::Piece::from_token("wX").has_value());
    EXPECT_FALSE(kfc::Piece::from_token("").has_value());
    EXPECT_FALSE(kfc::Piece::from_token("w").has_value());
    EXPECT_FALSE(kfc::Piece::from_token("abc").has_value());
}

TEST(PieceTest, PieceToToken) {
    EXPECT_EQ(kfc::Piece::empty().to_token(), ".");
    const std::optional<kfc::Piece> piece = kfc::Piece::from_token("bR");
    ASSERT_TRUE(piece.has_value());
    EXPECT_EQ(piece->to_token(), "bR");
}

TEST(PieceTest, PieceColorHelpers) {
    const kfc::Piece white = *kfc::Piece::from_token("wN");
    const kfc::Piece black = *kfc::Piece::from_token("bN");
    const kfc::Piece empty = kfc::Piece::empty();
    EXPECT_TRUE(white.is_white());
    EXPECT_FALSE(white.is_black());
    EXPECT_TRUE(black.is_black());
    EXPECT_TRUE(white.is_same_color_as(white));
    EXPECT_FALSE(white.is_same_color_as(black));
    EXPECT_FALSE(white.is_same_color_as(empty));
    EXPECT_TRUE(white.is_opponent_of(black));
    EXPECT_FALSE(white.is_opponent_of(white));
    EXPECT_NE(white, black);
    EXPECT_EQ(white, white);
}

TEST(MoveValidatorTest, BishopDiagonalMove) {
    const kfc::BoardModel board = make_board({{".", ".", "."}, {".", "wB", "."}, {".", ".", "."}});
    EXPECT_TRUE(kfc::is_legal_move(board, 'B', 1, 1, 0, 0));
    EXPECT_TRUE(kfc::is_legal_move(board, 'B', 1, 1, 2, 2));
    EXPECT_FALSE(kfc::is_legal_move(board, 'B', 1, 1, 1, 2));
    EXPECT_FALSE(kfc::is_legal_move(board, 'B', 1, 1, 2, 1));
}

TEST(MoveValidatorTest, BishopBlockedByPiece) {
    const kfc::BoardModel board = make_board({{"wP", ".", "."}, {".", "wB", "."}, {".", ".", "."}});
    EXPECT_FALSE(kfc::is_legal_move(board, 'B', 1, 1, 0, 0));
}

TEST(MoveValidatorTest, BishopCapturesEnemy) {
    const kfc::BoardModel board = make_board({{".", ".", "bP"}, {".", "wB", "."}, {".", ".", "."}});
    EXPECT_TRUE(kfc::is_legal_move(board, 'B', 1, 1, 0, 2));
}

TEST(MoveValidatorTest, QueenStraightAndDiagonalMoves) {
    const kfc::BoardModel board =
        make_board({{".", ".", ".", "."}, {".", "wQ", ".", "."}, {".", ".", ".", "."}});
    EXPECT_TRUE(kfc::is_legal_move(board, 'Q', 1, 1, 1, 3));
    EXPECT_TRUE(kfc::is_legal_move(board, 'Q', 1, 1, 0, 0));
    EXPECT_FALSE(kfc::is_legal_move(board, 'Q', 1, 1, 0, 3));
}

TEST(MoveValidatorTest, QueenBlockedByFriendlyPiece) {
    const kfc::BoardModel board = make_board({{".", "wP", ".", "."}, {".", "wQ", ".", "."}});
    EXPECT_FALSE(kfc::is_legal_move(board, 'Q', 1, 1, 0, 1));
}

TEST(PathUtilsTest, ForEachCellOnPath) {
    std::vector<std::pair<int, int>> cells;
    kfc::for_each_cell_on_path(0, 0, 0, 3, [&](int row, int col) {
        cells.emplace_back(row, col);
    });
    EXPECT_EQ(cells.size(), 2);
    EXPECT_EQ(cells[0], std::make_pair(0, 1));
    EXPECT_EQ(cells[1], std::make_pair(0, 2));

    cells.clear();
    kfc::for_each_cell_on_path(0, 0, 2, 2, [&](int row, int col) {
        cells.emplace_back(row, col);
    });
    EXPECT_EQ(cells.size(), 1);
    EXPECT_EQ(cells[0], std::make_pair(1, 1));
}

TEST(PathUtilsTest, PathsShareCellOnOverlappingRoutes) {
    EXPECT_TRUE(kfc::paths_share_cell({0, 0}, {0, 3}, {0, 1}, {0, 4}));
    EXPECT_TRUE(kfc::paths_share_cell({0, 0}, {2, 2}, {0, 2}, {2, 0}));
    EXPECT_FALSE(kfc::paths_share_cell({0, 0}, {0, 2}, {1, 0}, {1, 2}));
}

TEST(PathUtilsTest, PathsShareCellNonStraightPaths) {
    EXPECT_TRUE(kfc::paths_share_cell({0, 0}, {0, 2}, {0, 2}, {2, 2}));
    EXPECT_FALSE(kfc::paths_share_cell({0, 0}, {0, 1}, {2, 0}, {2, 1}));
}

TEST(CommandProcessorTest, CommandProcessorWaitWithoutMs) {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    EXPECT_EQ(state.clock_ms(), 0);
    processor.execute("wait", sink);
    EXPECT_EQ(state.clock_ms(), 0);
}

TEST(CommandProcessorTest, CommandProcessorUnknownVerb) {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("foobar 1 2", sink);
    EXPECT_FALSE(state.has_selection());
    EXPECT_EQ(state.clock_ms(), 0);
}

TEST(GameStateTest, GameStateCustomRules) {
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
    EXPECT_TRUE(state.is_piece_moving(0, 0));

    state.add_clock(1000);
    EXPECT_EQ(state.token_at(0, 2), "wR");
    EXPECT_FALSE(state.is_game_over());

    state.select(0, 2);
    state.jump_selected();
    EXPECT_TRUE(state.is_piece_jumping(0, 2));
    state.add_clock(300);
    EXPECT_FALSE(state.is_piece_jumping(0, 2));
}

TEST(CommandProcessorTest, JumpCommandTooLateDoesNotSavePiece) {
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
    EXPECT_EQ(output.str(), ". . .\nbR . .\n. . .");
}

TEST(BoardValidatorTest, IsValidTokenInvalidColor) {
    EXPECT_FALSE(kfc::is_valid_token("xK"));
    EXPECT_FALSE(kfc::is_valid_token("zP"));
}

TEST(PieceTest, PieceOpponentOfEmpty) {
    const kfc::Piece white = *kfc::Piece::from_token("wK");
    const kfc::Piece empty = kfc::Piece::empty();
    EXPECT_FALSE(white.is_opponent_of(empty));
    EXPECT_FALSE(empty.is_opponent_of(white));
}

TEST(MoveValidatorTest, IllegalMoveEmptyBoard) {
    const kfc::BoardModel board;
    EXPECT_FALSE(kfc::is_legal_move(board, 'K', 0, 0, 0, 1));
}

TEST(MoveValidatorTest, IllegalMoveSameCell) {
    const kfc::BoardModel board = make_board({{"wK", ".", "bK"}});
    EXPECT_FALSE(kfc::is_legal_move(board, 'K', 0, 0, 0, 0));
}

TEST(MoveValidatorTest, IllegalMoveUnknownPieceType) {
    const kfc::BoardModel board = make_board({{"wK", ".", "bK"}});
    EXPECT_FALSE(kfc::is_legal_move(board, 'X', 0, 0, 0, 1));
}

TEST(BoardValidatorTest, ParseBoardEmptyRowTokens) {
    const std::vector<std::string> lines = {""};
    kfc::BoardModel board;
    EXPECT_EQ(kfc::parse_board_rows(lines, board), kfc::BoardError::RowWidthMismatch);
}

TEST(BoardModelTest, BoardModelInBoundsEmptyBoard) {
    const kfc::BoardModel board;
    EXPECT_FALSE(board.is_in_bounds(0, 0));
    EXPECT_FALSE(board.contains(0, 0));
    EXPECT_FALSE(board.contains(5, 5));
}

TEST(BoardModelTest, BoardModelContainsOutOfBounds) {
    const kfc::BoardModel board = make_board({{"wK", ".", "bK"}});
    EXPECT_FALSE(board.contains(0, 3));
    EXPECT_FALSE(board.contains(1, 0));
}

TEST(BoardModelTest, BoardModelEqualityDifferentSizes) {
    const kfc::BoardModel narrow = make_board({{"wK"}});
    const kfc::BoardModel wide = make_board({{"wK", "."}});
    EXPECT_FALSE(narrow == wide);
}

TEST(BoardWriterTest, WriteEmptyBoard) {
    std::ostringstream output;
    kfc::write_board(output, kfc::BoardModel{});
    EXPECT_TRUE(output.str().empty());
}

TEST(BoardWriterTest, WriteSingleRowBoard) {
    std::ostringstream output;
    kfc::write_board(output, make_board({{"wK", "bK"}}));
    EXPECT_EQ(output.str(), "wK bK");
}

TEST(PathUtilsTest, ForEachCellOnPathAdjacent) {
    std::vector<std::pair<int, int>> cells;
    kfc::for_each_cell_on_path(0, 0, 0, 1, [&](int row, int col) {
        cells.emplace_back(row, col);
    });
    EXPECT_TRUE(cells.empty());
}

TEST(PathUtilsTest, ForEachCellOnPathVertical) {
    std::vector<std::pair<int, int>> cells;
    kfc::for_each_cell_on_path(0, 0, 3, 0, [&](int row, int col) {
        cells.emplace_back(row, col);
    });
    EXPECT_EQ(cells.size(), 2);
    EXPECT_EQ(cells[0], std::make_pair(1, 0));
    EXPECT_EQ(cells[1], std::make_pair(2, 0));
}

TEST(PathUtilsTest, PathsShareCellEndpointOverlap) {
    EXPECT_TRUE(kfc::paths_share_cell({0, 0}, {0, 1}, {0, 1}, {0, 2}));
}

TEST(CollisionResolverTest, CollisionHasCommonRouteHorizontalParallel) {
    const kfc::PendingMove left_to_right{
        *kfc::Piece::from_token("wR"), {0, 0}, {0, 4}, 1000};
    const kfc::PendingMove middle_to_right{
        *kfc::Piece::from_token("bR"), {0, 2}, {0, 6}, 1000};
    EXPECT_TRUE(kfc::CollisionResolver::has_common_route(left_to_right, middle_to_right));
}

TEST(CollisionResolverTest, CollisionHasCommonRouteVerticalParallel) {
    const kfc::PendingMove top_to_bottom{
        *kfc::Piece::from_token("wR"), {0, 0}, {4, 0}, 1000};
    const kfc::PendingMove middle_to_bottom{
        *kfc::Piece::from_token("bR"), {2, 0}, {6, 0}, 1000};
    EXPECT_TRUE(kfc::CollisionResolver::has_common_route(top_to_bottom, middle_to_bottom));
}

TEST(CollisionResolverTest, CollisionHasCommonRouteDisjoint) {
    const kfc::PendingMove left{
        *kfc::Piece::from_token("wR"), {0, 0}, {0, 1}, 1000};
    const kfc::PendingMove right{
        *kfc::Piece::from_token("bR"), {0, 3}, {0, 4}, 1000};
    EXPECT_FALSE(kfc::CollisionResolver::has_common_route(left, right));
}

TEST(CollisionResolverTest, CollisionConflictsWithOppositeColorMove) {
    std::vector<kfc::PendingMove> pending;
    pending.push_back({*kfc::Piece::from_token("bR"), {0, 2}, {0, 0}, 1000});
    const kfc::PendingMove proposed{*kfc::Piece::from_token("wR"), {0, 0}, {0, 2}, 1000};
    EXPECT_TRUE(kfc::CollisionResolver::conflicts_with_opposite_color_move(pending, 500, 'w', proposed));
    EXPECT_FALSE(kfc::CollisionResolver::conflicts_with_opposite_color_move(pending, 1500, 'w', proposed));
}

TEST(CollisionResolverTest, CollisionSameColorDestinationClaimed) {
    std::vector<kfc::PendingMove> pending;
    pending.push_back({*kfc::Piece::from_token("wN"), {0, 0}, {0, 2}, 1000});
    EXPECT_TRUE(kfc::CollisionResolver::is_same_color_destination_claimed(pending, 500, 'w', {0, 2}));
    EXPECT_FALSE(kfc::CollisionResolver::is_same_color_destination_claimed(pending, 500, 'b', {0, 2}));
    EXPECT_FALSE(kfc::CollisionResolver::is_same_color_destination_claimed(pending, 1500, 'w', {0, 2}));
}

TEST(CollisionResolverTest, CollisionNormalCaptureOnArrival) {
    kfc::BoardModel board = make_board({{"wR", ".", "bP"}});
    kfc::CollisionResolver resolver;
    const kfc::GameRules rules = kfc::KungFuChessRules::standard();
    bool game_over = false;
    const kfc::ArrivingPieceInfo arriving{
        *kfc::Piece::from_token("wR"), {0, 0}, {0, 2}};
    EXPECT_TRUE(resolver.check_for_jump_capture(board, rules, 1000, {}, {0, 2}, arriving, game_over));
    EXPECT_EQ(board.token_at(0, 2), "wR");
    EXPECT_FALSE(game_over);
}

TEST(CollisionResolverTest, CollisionEmptyDestinationNotCaptured) {
    kfc::BoardModel board = make_board({{"wR", ".", "."}});
    kfc::CollisionResolver resolver;
    const kfc::GameRules rules = kfc::KungFuChessRules::standard();
    bool game_over = false;
    const kfc::ArrivingPieceInfo arriving{
        *kfc::Piece::from_token("wR"), {0, 0}, {0, 2}};
    EXPECT_FALSE(resolver.check_for_jump_capture(board, rules, 1000, {}, {0, 2}, arriving, game_over));
    EXPECT_EQ(board.token_at(0, 0), "wR");
}

TEST(CollisionResolverTest, CollisionJumpCaptureSetsGameOver) {
    kfc::BoardModel board = make_board({{".", ".", "."}, {"wK", ".", "."}, {".", ".", "."}});
    kfc::CollisionResolver resolver;
    const kfc::GameRules rules = kfc::KungFuChessRules::standard();
    std::vector<kfc::JumpState> jumps;
    jumps.push_back({*kfc::Piece::from_token("wK"), {1, 0}, 2000});
    bool game_over = false;
    const kfc::ArrivingPieceInfo arriving{
        *kfc::Piece::from_token("bK"), {2, 0}, {1, 0}};
    EXPECT_TRUE(resolver.check_for_jump_capture(board, rules, 1000, jumps, {1, 0}, arriving, game_over));
    EXPECT_TRUE(game_over);
}

TEST(VplIoTest, ReadVplMultipleCommandsAndSkipBlankLines) {
    const std::string input =
        "Board:\n"
        "wK . bK\n"
        "Commands:\n"
        "wait 100\n"
        "\n"
        "print board\n";

    std::istringstream in(input);
    const kfc::VplInput parsed = kfc::read_vpl_input(in);
    EXPECT_EQ(parsed.error, kfc::BoardError::Ok);
    EXPECT_EQ(parsed.commands.size(), 2);
    EXPECT_EQ(parsed.commands[0], "wait 100");
    EXPECT_EQ(parsed.commands[1], "print board");
}

TEST(VplIoTest, ReadVplNoSectionHeaders) {
    const std::string input = "wK . bK\n";
    std::istringstream in(input);
    const kfc::VplInput parsed = kfc::read_vpl_input(in);
    EXPECT_EQ(parsed.error, kfc::BoardError::Ok);
    EXPECT_EQ(parsed.board.rows(), 0);
    EXPECT_TRUE(parsed.commands.empty());
}

TEST(VplIoTest, ReadVplWhitespacePaddedBoardLine) {
    const std::string input =
        "Board:\n"
        "  wK . bK  \n"
        "Commands:\n";

    std::istringstream in(input);
    const kfc::VplInput parsed = kfc::read_vpl_input(in);
    EXPECT_EQ(parsed.error, kfc::BoardError::Ok);
    EXPECT_EQ(parsed.board.token_at(0, 0), "wK");
    EXPECT_EQ(parsed.board.token_at(0, 2), "bK");
}

TEST(CommandProcessorTest, CommandProcessorClickWithoutCoords) {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click", sink);
    EXPECT_FALSE(state.has_selection());
}

TEST(CommandProcessorTest, CommandProcessorJumpWithoutCoords) {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("jump", sink);
    EXPECT_FALSE(state.is_piece_jumping(0, 0));
}

TEST(CommandProcessorTest, CommandProcessorPrintPartialCommandIgnored) {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    kfc::CommandProcessor processor(state);
    std::ostringstream output;

    processor.execute("print", output);
    EXPECT_TRUE(output.str().empty());
    processor.execute("print foo", output);
    EXPECT_TRUE(output.str().empty());
}

TEST(CommandProcessorTest, CommandProcessorSelectEmptySquare) {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 150 50", sink);
    EXPECT_FALSE(state.has_selection());
}

TEST(CommandProcessorTest, CommandProcessorFriendlyClickSameCellJumps) {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    EXPECT_TRUE(state.has_selection());
    processor.execute("click 50 50", sink);
    EXPECT_FALSE(state.has_selection());
    EXPECT_TRUE(state.is_piece_jumping(0, 0));
}

TEST(CommandProcessorTest, CommandProcessorJumpOutsideGrid) {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("jump 350 50", sink);
    EXPECT_FALSE(state.is_piece_jumping(0, 0));
}

TEST(CommandProcessorTest, CommandProcessorJumpOnEmptyCell) {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("jump 150 50", sink);
    EXPECT_FALSE(state.is_piece_jumping(0, 1));
}

TEST(GameStateTest, GameStateSelectOutOfBounds) {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    state.select(99, 99);
    EXPECT_FALSE(state.has_selection());
}

TEST(GameStateTest, GameStateMoveAndJumpWithoutSelection) {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    state.move_selected_to(0, 1);
    state.jump_selected();
    EXPECT_EQ(state.token_at(0, 0), "wK");
    EXPECT_FALSE(state.is_piece_jumping(0, 0));
}

TEST(GameStateTest, GameStateJumpAtEmptyCell) {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    state.jump_at(0, 1);
    EXPECT_FALSE(state.is_piece_jumping(0, 1));
}

TEST(GameStateTest, GameStateIsPieceOutOfBounds) {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    EXPECT_FALSE(state.is_piece(99, 99));
}

TEST(GameStateTest, GameStateFriendlySelectionRequiresSelection) {
    kfc::GameState state(make_board({{"wK", "wN", "bK"}}));
    EXPECT_FALSE(state.is_friendly_to_selection(0, 1));
}

TEST(GameStateTest, GameStateSameBoardLayoutAs) {
    const kfc::GameState left(make_board({{"wK", ".", "bK"}}));
    const kfc::GameState right(make_board({{"wK", ".", "bK"}}));
    const kfc::GameState different(make_board({{"wK", ".", "."}}));
    EXPECT_TRUE(left.same_board_layout_as(right));
    EXPECT_FALSE(left.same_board_layout_as(different));
}

TEST(GameStateTest, GameStateCaptureMoveUsesSingleCellDuration) {
    kfc::GameState state(make_board({{"wR", ".", "bP"}}));
    state.select(0, 0);
    state.move_selected_to(0, 2);
    state.add_clock(999);
    EXPECT_EQ(state.token_at(0, 0), "wR");
    EXPECT_EQ(state.token_at(0, 2), "bP");
    state.add_clock(1);
    EXPECT_EQ(state.token_at(0, 0), ".");
    EXPECT_EQ(state.token_at(0, 2), "wR");
}

TEST(GameRulesTest, StandardRulesConfiguration) {
    const kfc::GameRules rules = kfc::KungFuChessRules::standard();
    EXPECT_EQ(rules.move_duration_ms, kfc::kMoveDurationMs);
    EXPECT_EQ(rules.jump_duration_ms, kfc::kJumpDurationMs);
    EXPECT_TRUE(rules.is_game_over(*kfc::Piece::from_token("wK")));
    EXPECT_TRUE(rules.is_game_over(*kfc::Piece::from_token("bK")));
    EXPECT_FALSE(rules.is_game_over(*kfc::Piece::from_token("wP")));

    const kfc::Piece white_pawn = *kfc::Piece::from_token("wP");
    EXPECT_EQ(rules.on_reach_last_row(white_pawn, 0, 8).type, kfc::kQueenType);
    EXPECT_EQ(rules.on_reach_last_row(white_pawn, 1, 8).type, kfc::kPawnType);
    EXPECT_EQ(rules.on_reach_last_row(*kfc::Piece::from_token("wR"), 0, 8).type, kfc::kRookType);

    const kfc::Piece black_pawn = *kfc::Piece::from_token("bP");
    EXPECT_EQ(rules.on_reach_last_row(black_pawn, 7, 8).type, kfc::kQueenType);
}

TEST(BoardModelTest, ValidRectangularBoard) {
    const kfc::BoardModel board =
        make_board({{"wK", ".", "bK"}, {".", "wN", "."}, {"bP", ".", "wR"}});
    EXPECT_TRUE(board.is_valid());
}

}  // namespace

