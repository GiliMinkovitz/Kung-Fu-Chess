#include "board.h"

#include <cassert>
#include <sstream>
#include <string>

namespace {

void test_valid_rectangular_board() {
    const kfc::Board board = {{"wK", ".", "bK"}, {".", "wN", "."}, {"bP", ".", "wR"}};
    assert(kfc::is_valid_board(board));
}

void test_invalid_empty_board() {
    const kfc::Board board;
    assert(!kfc::is_valid_board(board));
}

void test_invalid_non_rectangular_board() {
    const kfc::Board board = {{"wK", "."}, {"bK"}};
    assert(!kfc::is_valid_board(board));
}

void test_invalid_unknown_token() {
    assert(!kfc::is_valid_token("xZ"));
    assert(kfc::is_valid_token("."));
    assert(kfc::is_valid_token("wK"));
    assert(kfc::is_valid_token("bQ"));
}

void test_parse_board_rows_success() {
    const std::vector<std::string> lines = {"wK . . bK", ". . . .", "wR . . bR"};
    kfc::Board board;
    assert(kfc::parse_board_rows(lines, board) == kfc::BoardError::Ok);
    assert(board.size() == 3);
    assert(board[0].size() == 4);
    assert(board[0][0] == "wK");
    assert(board[2][3] == "bR");
}

void test_parse_board_unknown_token() {
    const std::vector<std::string> lines = {"wK xZ", ". ."};
    kfc::Board board;
    assert(kfc::parse_board_rows(lines, board) == kfc::BoardError::UnknownToken);
}

void test_parse_board_row_width_mismatch() {
    const std::vector<std::string> lines = {"wK . .", ". bK"};
    kfc::Board board;
    assert(kfc::parse_board_rows(lines, board) == kfc::BoardError::RowWidthMismatch);
}

void test_read_and_write_roundtrip() {
    const std::string input =
        "Board:\n"
        "wK . bQ\n"
        ". wN .\n"
        "bP . wR\n"
        "Commands:\n"
        "print board\n";

    std::istringstream in(input);
    const kfc::VplInput parsed = kfc::read_vpl_input(in);
    assert(parsed.error == kfc::BoardError::Ok);
    assert(kfc::is_valid_board(parsed.board));
    assert(parsed.commands.size() == 1);
    assert(parsed.commands[0] == "print board");

    std::ostringstream output;
    kfc::write_board(output, parsed.board);
    assert(output.str() == "wK . bQ\n. wN .\nbP . wR");
}

void test_invalid_board_produces_error() {
    const std::string input =
        "Board:\n"
        "wK xZ\n"
        ". .\n"
        "Commands:\n";

    std::istringstream in(input);
    const kfc::VplInput parsed = kfc::read_vpl_input(in);
    assert(parsed.error == kfc::BoardError::UnknownToken);
    assert(std::string(kfc::board_error_message(parsed.error)) == "ERROR UNKNOWN_TOKEN");
}

}  // namespace

int main() {
    test_valid_rectangular_board();
    test_invalid_empty_board();
    test_invalid_non_rectangular_board();
    test_invalid_unknown_token();
    test_parse_board_rows_success();
    test_parse_board_unknown_token();
    test_parse_board_row_width_mismatch();
    test_read_and_write_roundtrip();
    test_invalid_board_produces_error();
    return 0;
}
