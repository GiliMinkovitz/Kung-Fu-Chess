#include "board.h"

#include <cassert>
#include <sstream>
#include <string>

namespace {

void test_valid_rectangular_board() {
    const kfc::Board board = {"abc", "def", "ghi"};
    assert(kfc::is_valid_board(board));
}

void test_invalid_empty_board() {
    const kfc::Board board;
    assert(!kfc::is_valid_board(board));
}

void test_invalid_non_rectangular_board() {
    const kfc::Board board = {"abc", "de", "fghi"};
    assert(!kfc::is_valid_board(board));
}

void test_invalid_zero_width_row() {
    const kfc::Board board = {""};
    assert(!kfc::is_valid_board(board));
}

void test_read_and_write_roundtrip() {
    std::istringstream input("rnbqkbnr\n........\n");
    kfc::Board board;
    kfc::read_board(input, board);

    assert(kfc::is_valid_board(board));
    assert(board.size() == 2);
    assert(board[0] == "rnbqkbnr");
    assert(board[1] == "........");

    std::ostringstream output;
    kfc::write_board(output, board);
    assert(output.str() == "rnbqkbnr\n........\n");
}

void test_invalid_board_produces_no_output() {
    const kfc::Board board = {"abc", "de"};

    std::ostringstream output;
    if (kfc::is_valid_board(board)) {
        kfc::write_board(output, board);
    }

    assert(output.str().empty());
}

}  // namespace

int main() {
    test_valid_rectangular_board();
    test_invalid_empty_board();
    test_invalid_non_rectangular_board();
    test_invalid_zero_width_row();
    test_read_and_write_roundtrip();
    test_invalid_board_produces_no_output();
    return 0;
}
