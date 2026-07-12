#include "core/board_model.h"
#include "test_helpers.h"

#include <doctest/doctest.h>
#include <stdexcept>

TEST_CASE("BoardModelTest - InvalidEmptyBoard") {
    const kfc::BoardModel board;
    CHECK_FALSE(board.is_valid());
}

TEST_CASE("BoardModelTest - InvalidNonRectangularBoard") {
    const kfc::BoardModel board = kfc::test::make_board({{"wK", "."}, {"bK"}});
    CHECK_FALSE(board.is_valid());
}

TEST_CASE("BoardModelTest - BoardModelEquality") {
    const kfc::BoardModel left = kfc::test::make_board({{"wK", ".", "bK"}});
    const kfc::BoardModel same = kfc::test::make_board({{"wK", ".", "bK"}});
    const kfc::BoardModel different = kfc::test::make_board({{"wK", ".", "."}});
    CHECK_EQ(left, same);
    CHECK_FALSE(left == different);
}

TEST_CASE("BoardModelTest - BoardModelContainsNegative") {
    const kfc::BoardModel board = kfc::test::make_board({{"wK", ".", "bK"}});
    CHECK_FALSE(board.contains(-1, 0));
    CHECK_FALSE(board.contains(0, -1));
    CHECK(board.contains(0, 0));
    CHECK(board.contains(0, 2));
}

TEST_CASE("BoardModelTest - BoardModelInBoundsEmptyBoard") {
    const kfc::BoardModel board;
    CHECK_FALSE(board.is_in_bounds(0, 0));
    CHECK_FALSE(board.contains(0, 0));
    CHECK_FALSE(board.contains(5, 5));
}

TEST_CASE("BoardModelTest - BoardModelContainsOutOfBounds") {
    const kfc::BoardModel board = kfc::test::make_board({{"wK", ".", "bK"}});
    CHECK_FALSE(board.contains(0, 3));
    CHECK_FALSE(board.contains(1, 0));
}

TEST_CASE("BoardModelTest - BoardModelEqualityDifferentSizes") {
    const kfc::BoardModel narrow = kfc::test::make_board({{"wK"}});
    const kfc::BoardModel wide = kfc::test::make_board({{"wK", "."}});
    CHECK_FALSE(narrow == wide);
}

TEST_CASE("BoardModelTest - ValidRectangularBoard") {
    const kfc::BoardModel board =
        kfc::test::make_board({{"wK", ".", "bK"}, {".", "wN", "."}, {"bP", ".", "wR"}});
    CHECK(board.is_valid());
}

TEST_CASE("BoardModelTest - GetPieceInvalidIdThrows") {
    kfc::BoardModel board = kfc::test::make_board({{"wK"}});
    CHECK_THROWS_AS(
        [&board]() -> kfc::Piece& { return board.get_piece(999); }(), std::out_of_range);
    CHECK_THROWS_AS(
        [&board]() -> kfc::Piece& { return board.get_piece(kfc::Piece::kInvalidId); }(),
        std::out_of_range);
}

TEST_CASE("BoardModelTest - PlacePieceInvalidIdThrows") {
    kfc::BoardModel board;
    kfc::Piece invalid = kfc::test::make_piece(kfc::PieceColor::White, kfc::PieceKind::King);
    invalid.id = 99;
    CHECK_THROWS_AS(board.place_piece(std::move(invalid)), std::invalid_argument);
}

TEST_CASE("BoardModelTest - FindPieceByIdReturnsNullForInvalidId") {
    kfc::BoardModel board = kfc::test::make_board({{"wK"}});
    CHECK(board.piece_at(0, 0) != nullptr);
    board.clear_cell(0, 0);
    CHECK(board.piece_at(0, 0) == nullptr);
}
