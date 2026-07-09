#include "core/board_model.h"
#include "logic/move_validator.h"
#include "test_helpers.h"

#include <doctest/doctest.h>

TEST_CASE("MoveValidatorTest - KnightLegalLMove") {
    const kfc::BoardModel board = kfc::test::make_board({{".", ".", ".", ".", "."},
                              {".", ".", ".", ".", "."},
                              {".", ".", "wN", ".", "."},
                              {".", ".", ".", ".", "."},
                              {".", ".", ".", ".", "."}});
    CHECK(kfc::is_legal_move(board, 'N', 2, 2, 4, 3));
    CHECK(kfc::is_legal_move(board, 'N', 2, 2, 0, 3));
    CHECK_FALSE(kfc::is_legal_move(board, 'N', 2, 2, 2, 4));
}

TEST_CASE("MoveValidatorTest - RookCannotMoveDiagonally") {
    const kfc::BoardModel board = kfc::test::make_board({{".", ".", "."}, {".", "wR", "."}, {".", ".", "."}});
    CHECK_FALSE(kfc::is_legal_move(board, 'R', 1, 1, 2, 2));
    CHECK_FALSE(kfc::is_legal_move(board, 'R', 1, 1, 0, 0));
    CHECK(kfc::is_legal_move(board, 'R', 1, 1, 1, 0));
    CHECK(kfc::is_legal_move(board, 'R', 1, 1, 2, 1));
}

TEST_CASE("MoveValidatorTest - KingCannotMoveMoreThanOneSquare") {
    const kfc::BoardModel board = kfc::test::make_board({{".", ".", ".", "."}, {".", "wK", ".", "."}, {".", ".", ".", "."}});
    CHECK_FALSE(kfc::is_legal_move(board, 'K', 1, 1, 1, 3));
    CHECK_FALSE(kfc::is_legal_move(board, 'K', 1, 1, 3, 1));
    CHECK_FALSE(kfc::is_legal_move(board, 'K', 1, 1, 3, 3));
    CHECK(kfc::is_legal_move(board, 'K', 1, 1, 1, 2));
    CHECK(kfc::is_legal_move(board, 'K', 1, 1, 2, 2));
}

TEST_CASE("MoveValidatorTest - MoveRespectsBoardBoundaries") {
    const kfc::BoardModel board = kfc::test::make_board({{"wN", ".", "wR"}, {".", "wK", "."}});
    CHECK_FALSE(kfc::is_legal_move(board, 'N', 0, 0, -1, 1));
    CHECK_FALSE(kfc::is_legal_move(board, 'R', 0, 2, 0, 5));
    CHECK_FALSE(kfc::is_legal_move(board, 'K', 1, 1, 2, 3));
}

TEST_CASE("MoveValidatorTest - RookBlockedByFriendlyPiece") {
    const kfc::BoardModel board = kfc::test::make_board({{".", ".", ".", "."}, {".", "wR", "wP", "."}});
    CHECK_FALSE(kfc::is_legal_move(board, 'R', 1, 1, 1, 3));
    CHECK(kfc::is_legal_move(board, 'R', 1, 1, 1, 0));
}

TEST_CASE("MoveValidatorTest - RookCapturesEnemyPiece") {
    const kfc::BoardModel board = kfc::test::make_board({{".", ".", ".", "."}, {".", "wR", ".", "bP"}});
    CHECK(kfc::is_legal_move(board, 'R', 1, 1, 1, 3));
}

TEST_CASE("MoveValidatorTest - KnightJumpsOverPieces") {
    const kfc::BoardModel board = kfc::test::make_board({{".", ".", ".", ".", "."},
                              {".", "wP", "bN", ".", "."},
                              {".", ".", "wN", ".", "."},
                              {".", ".", ".", ".", "."},
                              {".", ".", ".", ".", "."}});
    CHECK(kfc::is_legal_move(board, 'N', 2, 2, 0, 3));
    CHECK(kfc::is_legal_move(board, 'N', 2, 2, 4, 1));
}

TEST_CASE("MoveValidatorTest - CannotCaptureOwnPiece") {
    const kfc::BoardModel board = kfc::test::make_board({{".", "wP", "wR"}});
    CHECK_FALSE(kfc::is_legal_move(board, 'R', 0, 2, 0, 1));
    CHECK_FALSE(kfc::is_legal_move(board, 'N', 0, 2, 0, 1));
}

TEST_CASE("MoveValidatorTest - WhitePawnForwardMove") {
    const kfc::BoardModel board = kfc::test::make_board({{".", ".", "."}, {".", "wP", "."}, {".", ".", "."}});
    CHECK(kfc::is_legal_move(board, 'P', 1, 1, 0, 1));
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 1, 1, 0, 0));
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 1, 1, 0, 2));
}

TEST_CASE("MoveValidatorTest - WhitePawnBlockedForward") {
    const kfc::BoardModel board = kfc::test::make_board({{".", "bK", "."}, {".", "wP", "."}});
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 1, 1, 0, 1));
}

TEST_CASE("MoveValidatorTest - WhitePawnDiagonalCapture") {
    const kfc::BoardModel board = kfc::test::make_board({{"bN", ".", "bR"}, {".", "wP", "."}, {".", ".", "."}});
    CHECK(kfc::is_legal_move(board, 'P', 1, 1, 0, 0));
    CHECK(kfc::is_legal_move(board, 'P', 1, 1, 0, 2));

    const kfc::BoardModel empty_diagonal = kfc::test::make_board({{".", ".", "."}, {".", "wP", "."}});
    CHECK_FALSE(kfc::is_legal_move(empty_diagonal, 'P', 1, 1, 0, 0));
    CHECK_FALSE(kfc::is_legal_move(empty_diagonal, 'P', 1, 1, 0, 2));
}

TEST_CASE("MoveValidatorTest - WhitePawnCannotMoveBackwardOrForwardCapture") {
    const kfc::BoardModel board = kfc::test::make_board({{".", "wP", "."}, {".", "bK", "."}, {".", ".", "."}});
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 0, 1, 1, 1));
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 0, 1, 1, 0));
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 0, 1, 1, 2));
}

TEST_CASE("MoveValidatorTest - WhitePawnCannotCaptureOwnPiece") {
    const kfc::BoardModel board = kfc::test::make_board({{"wN", ".", "wR"}, {".", "wP", "."}});
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 1, 1, 0, 0));
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 1, 1, 0, 2));
}

TEST_CASE("MoveValidatorTest - BlackPawnForwardMove") {
    const kfc::BoardModel board = kfc::test::make_board({{".", ".", "."}, {".", "bP", "."}, {".", ".", "."}});
    CHECK(kfc::is_legal_move(board, 'P', 1, 1, 2, 1));
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 1, 1, 2, 0));
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 1, 1, 2, 2));
}

TEST_CASE("MoveValidatorTest - BlackPawnBlockedForward") {
    const kfc::BoardModel board = kfc::test::make_board({{".", "bP", "."}, {".", "wK", "."}});
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 0, 1, 1, 1));
}

TEST_CASE("MoveValidatorTest - BlackPawnDiagonalCapture") {
    const kfc::BoardModel board = kfc::test::make_board({{".", ".", "."}, {".", "bP", "."}, {"wR", ".", "wN"}});
    CHECK(kfc::is_legal_move(board, 'P', 1, 1, 2, 0));
    CHECK(kfc::is_legal_move(board, 'P', 1, 1, 2, 2));

    const kfc::BoardModel empty_diagonal = kfc::test::make_board({{".", "bP", "."}, {".", ".", "."}});
    CHECK_FALSE(kfc::is_legal_move(empty_diagonal, 'P', 0, 1, 1, 0));
    CHECK_FALSE(kfc::is_legal_move(empty_diagonal, 'P', 0, 1, 1, 2));
}

TEST_CASE("MoveValidatorTest - BlackPawnCannotMoveBackwardOrForwardCapture") {
    const kfc::BoardModel board = kfc::test::make_board({{".", ".", "."}, {".", "wK", "."}, {".", "bP", "."}});
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 2, 1, 1, 1));
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 2, 1, 1, 0));
    CHECK_FALSE(kfc::is_legal_move(board, 'P', 2, 1, 1, 2));
}

TEST_CASE("MoveValidatorTest - PawnDoubleMoveFromStartRow") {
    const kfc::BoardModel white_board = kfc::test::make_board({{".", ".", "."},
                                    {".", ".", "."},
                                    {".", ".", "."},
                                    {".", "wP", "."}});
    CHECK(kfc::is_legal_move(white_board, 'P', 3, 1, 1, 1));
    CHECK_FALSE(kfc::is_legal_move(white_board, 'P', 2, 1, 0, 1));

    const kfc::BoardModel black_board = kfc::test::make_board({{".", "bP", "."},
                                    {".", ".", "."},
                                    {".", ".", "."},
                                    {".", ".", "."}});
    CHECK(kfc::is_legal_move(black_board, 'P', 0, 1, 2, 1));
    CHECK_FALSE(kfc::is_legal_move(black_board, 'P', 1, 1, 3, 1));
}

TEST_CASE("MoveValidatorTest - PawnDoubleMoveBlockedByIntermediatePiece") {
    const kfc::BoardModel blocked_intermediate = kfc::test::make_board({{".", "bP", "."},
                                             {".", "wN", "."},
                                             {".", ".", "."},
                                             {".", ".", "."}});
    CHECK_FALSE(kfc::is_legal_move(blocked_intermediate, 'P', 0, 1, 2, 1));

    const kfc::BoardModel blocked_dest = kfc::test::make_board({{".", "bP", "."},
                                     {".", ".", "."},
                                     {".", "wR", "."},
                                     {".", ".", "."}});
    CHECK_FALSE(kfc::is_legal_move(blocked_dest, 'P', 0, 1, 2, 1));
}

TEST_CASE("MoveValidatorTest - BishopDiagonalMove") {
    const kfc::BoardModel board = kfc::test::make_board({{".", ".", "."}, {".", "wB", "."}, {".", ".", "."}});
    CHECK(kfc::is_legal_move(board, 'B', 1, 1, 0, 0));
    CHECK(kfc::is_legal_move(board, 'B', 1, 1, 2, 2));
    CHECK_FALSE(kfc::is_legal_move(board, 'B', 1, 1, 1, 2));
    CHECK_FALSE(kfc::is_legal_move(board, 'B', 1, 1, 2, 1));
}

TEST_CASE("MoveValidatorTest - BishopBlockedByPiece") {
    const kfc::BoardModel board = kfc::test::make_board({{"wP", ".", "."}, {".", "wB", "."}, {".", ".", "."}});
    CHECK_FALSE(kfc::is_legal_move(board, 'B', 1, 1, 0, 0));
}

TEST_CASE("MoveValidatorTest - BishopCapturesEnemy") {
    const kfc::BoardModel board = kfc::test::make_board({{".", ".", "bP"}, {".", "wB", "."}, {".", ".", "."}});
    CHECK(kfc::is_legal_move(board, 'B', 1, 1, 0, 2));
}

TEST_CASE("MoveValidatorTest - QueenStraightAndDiagonalMoves") {
    const kfc::BoardModel board =
        kfc::test::make_board({{".", ".", ".", "."}, {".", "wQ", ".", "."}, {".", ".", ".", "."}});
    CHECK(kfc::is_legal_move(board, 'Q', 1, 1, 1, 3));
    CHECK(kfc::is_legal_move(board, 'Q', 1, 1, 0, 0));
    CHECK_FALSE(kfc::is_legal_move(board, 'Q', 1, 1, 0, 3));
}

TEST_CASE("MoveValidatorTest - QueenBlockedByFriendlyPiece") {
    const kfc::BoardModel board = kfc::test::make_board({{".", "wP", ".", "."}, {".", "wQ", ".", "."}});
    CHECK_FALSE(kfc::is_legal_move(board, 'Q', 1, 1, 0, 1));
}

TEST_CASE("MoveValidatorTest - IllegalMoveEmptyBoard") {
    const kfc::BoardModel board;
    CHECK_FALSE(kfc::is_legal_move(board, 'K', 0, 0, 0, 1));
}

TEST_CASE("MoveValidatorTest - IllegalMoveSameCell") {
    const kfc::BoardModel board = kfc::test::make_board({{"wK", ".", "bK"}});
    CHECK_FALSE(kfc::is_legal_move(board, 'K', 0, 0, 0, 0));
}

TEST_CASE("MoveValidatorTest - IllegalMoveUnknownPieceType") {
    const kfc::BoardModel board = kfc::test::make_board({{"wK", ".", "bK"}});
    CHECK_FALSE(kfc::is_legal_move(board, 'X', 0, 0, 0, 1));
}
