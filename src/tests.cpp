#include "board.h"
#include "command_processor.h"
#include "game_state.h"
#include "move_validator.h"

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

void test_game_state_wait_increments_clock() {
    kfc::GameState state({{"wK", ".", "bK"}});
    assert(state.clock_ms() == 0);
    state.add_clock(250);
    state.add_clock(50);
    assert(state.clock_ms() == 300);
}

void test_command_processor_click_select_and_move() {
    kfc::Board board = {{"wK", ".", "bK"}, {".", ".", "."}};
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    assert(state.has_selection());
    std::size_t row = 0;
    std::size_t col = 0;
    assert(state.selection(row, col));
    assert(row == 0);
    assert(col == 0);

    processor.execute("click 150 150", sink);
    assert(!state.has_selection());
    assert(state.board()[0][0] == ".");
    assert(state.board()[1][1] == "wK");
}

void test_command_processor_click_outside_grid_ignored() {
    kfc::GameState state({{"wK", ".", "bK"}});
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 350 50", sink);
    processor.execute("click -10 50", sink);
    assert(!state.has_selection());
}

void test_command_processor_friendly_click_replaces_selection() {
    kfc::Board board = {{"wK", "wN", "bK"}};
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 150 50", sink);
    assert(state.has_selection());
    std::size_t row = 0;
    std::size_t col = 0;
    assert(state.selection(row, col));
    assert(row == 0);
    assert(col == 1);
    assert(state.board()[0][0] == "wK");
    assert(state.board()[0][1] == "wN");
}

void test_command_processor_print_board() {
    kfc::GameState state({{"wK", ".", "bK"}});
    kfc::CommandProcessor processor(state);

    std::ostringstream output;
    processor.execute("print board", output);
    assert(output.str() == "wK . bK");
}

void test_knight_legal_l_move() {
    const kfc::Board board = {{".", ".", ".", ".", "."},
                              {".", ".", ".", ".", "."},
                              {".", ".", "wN", ".", "."},
                              {".", ".", ".", ".", "."},
                              {".", ".", ".", ".", "."}};
    assert(kfc::is_legal_move(board, 'N', 2, 2, 4, 3));
    assert(kfc::is_legal_move(board, 'N', 2, 2, 0, 3));
    assert(!kfc::is_legal_move(board, 'N', 2, 2, 2, 4));
}

void test_rook_cannot_move_diagonally() {
    const kfc::Board board = {{".", ".", "."}, {".", "wR", "."}, {".", ".", "."}};
    assert(!kfc::is_legal_move(board, 'R', 1, 1, 2, 2));
    assert(!kfc::is_legal_move(board, 'R', 1, 1, 0, 0));
    assert(kfc::is_legal_move(board, 'R', 1, 1, 1, 0));
    assert(kfc::is_legal_move(board, 'R', 1, 1, 2, 1));
}

void test_king_cannot_move_more_than_one_square() {
    const kfc::Board board = {{".", ".", ".", "."}, {".", "wK", ".", "."}, {".", ".", ".", "."}};
    assert(!kfc::is_legal_move(board, 'K', 1, 1, 1, 3));
    assert(!kfc::is_legal_move(board, 'K', 1, 1, 3, 1));
    assert(!kfc::is_legal_move(board, 'K', 1, 1, 3, 3));
    assert(kfc::is_legal_move(board, 'K', 1, 1, 1, 2));
    assert(kfc::is_legal_move(board, 'K', 1, 1, 2, 2));
}

void test_move_respects_board_boundaries() {
    const kfc::Board board = {{"wN", ".", "wR"}, {".", "wK", "."}};
    assert(!kfc::is_legal_move(board, 'N', 0, 0, -1, 1));
    assert(!kfc::is_legal_move(board, 'R', 0, 2, 0, 5));
    assert(!kfc::is_legal_move(board, 'K', 1, 1, 2, 3));
}

void test_rook_blocked_by_friendly_piece() {
    const kfc::Board board = {{".", ".", ".", "."}, {".", "wR", "wP", "."}};
    assert(!kfc::is_legal_move(board, 'R', 1, 1, 1, 3));
    assert(kfc::is_legal_move(board, 'R', 1, 1, 1, 0));
}

void test_rook_captures_enemy_piece() {
    const kfc::Board board = {{".", ".", ".", "."}, {".", "wR", ".", "bP"}};
    assert(kfc::is_legal_move(board, 'R', 1, 1, 1, 3));
}

void test_knight_jumps_over_pieces() {
    const kfc::Board board = {{".", ".", ".", ".", "."},
                              {".", "wP", "bN", ".", "."},
                              {".", ".", "wN", ".", "."},
                              {".", ".", ".", ".", "."},
                              {".", ".", ".", ".", "."}};
    assert(kfc::is_legal_move(board, 'N', 2, 2, 0, 3));
    assert(kfc::is_legal_move(board, 'N', 2, 2, 4, 1));
}

void test_cannot_capture_own_piece() {
    const kfc::Board board = {{".", "wP", "wR"}};
    assert(!kfc::is_legal_move(board, 'R', 0, 2, 0, 1));
    assert(!kfc::is_legal_move(board, 'N', 0, 2, 0, 1));
}

void test_command_processor_capture() {
    kfc::Board board = {{"wR", ".", "bK"}};
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    assert(state.has_selection());

    processor.execute("click 250 50", sink);
    assert(!state.has_selection());
    assert(state.board()[0][0] == ".");
    assert(state.board()[0][2] == "wR");
}

void test_command_processor_rejects_illegal_move() {
    kfc::Board board = {{"wK", ".", ".", "."}, {".", ".", ".", "."}};
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    assert(state.has_selection());

    processor.execute("click 350 50", sink);
    assert(state.has_selection());
    assert(state.board()[0][0] == "wK");
    assert(state.board()[0][3] == ".");
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
    test_game_state_wait_increments_clock();
    test_command_processor_click_select_and_move();
    test_command_processor_click_outside_grid_ignored();
    test_command_processor_friendly_click_replaces_selection();
    test_command_processor_print_board();
    test_knight_legal_l_move();
    test_rook_cannot_move_diagonally();
    test_king_cannot_move_more_than_one_square();
    test_move_respects_board_boundaries();
    test_rook_blocked_by_friendly_piece();
    test_rook_captures_enemy_piece();
    test_knight_jumps_over_pieces();
    test_cannot_capture_own_piece();
    test_command_processor_capture();
    test_command_processor_rejects_illegal_move();
    return 0;
}
