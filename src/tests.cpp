#include "board_model.h"
#include "command_processor.h"
#include "game_state.h"
#include "move_validator.h"
#include "piece.h"
#include "vpl_io.h"

#include <cassert>
#include <sstream>
#include <string>

namespace {

kfc::BoardModel make_board(std::initializer_list<std::initializer_list<const char*>> rows) {
    return kfc::BoardModel::from_token_grid(rows);
}

void assert_token(const kfc::BoardModel& board, std::size_t row, std::size_t col,
                  const char* token) {
    assert(board.token_at(row, col) == token);
}

void assert_token(const kfc::GameState& state, std::size_t row, std::size_t col,
                  const char* token) {
    assert(state.board().token_at(row, col) == token);
}

void test_valid_rectangular_board() {
    const kfc::BoardModel board =
        make_board({{"wK", ".", "bK"}, {".", "wN", "."}, {"bP", ".", "wR"}});
    assert(board.is_valid());
}

void test_invalid_empty_board() {
    const kfc::BoardModel board;
    assert(!board.is_valid());
}

void test_invalid_non_rectangular_board() {
    const kfc::BoardModel board = make_board({{"wK", "."}, {"bK"}});
    assert(!board.is_valid());
}

void test_invalid_unknown_token() {
    assert(!kfc::is_valid_token("xZ"));
    assert(kfc::is_valid_token("."));
    assert(kfc::is_valid_token("wK"));
    assert(kfc::is_valid_token("bQ"));
}

void test_parse_board_rows_success() {
    const std::vector<std::string> lines = {"wK . . bK", ". . . .", "wR . . bR"};
    kfc::BoardModel board;
    assert(kfc::parse_board_rows(lines, board) == kfc::BoardError::Ok);
    assert(board.rows() == 3);
    assert(board.cols() == 4);
    assert_token(board, 0, 0, "wK");
    assert_token(board, 2, 3, "bR");
}

void test_parse_board_unknown_token() {
    const std::vector<std::string> lines = {"wK xZ", ". ."};
    kfc::BoardModel board;
    assert(kfc::parse_board_rows(lines, board) == kfc::BoardError::UnknownToken);
}

void test_parse_board_row_width_mismatch() {
    const std::vector<std::string> lines = {"wK . .", ". bK"};
    kfc::BoardModel board;
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
    assert(parsed.board.is_valid());
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
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    assert(state.clock_ms() == 0);
    state.add_clock(250);
    state.add_clock(50);
    assert(state.clock_ms() == 300);
}

void test_command_processor_click_select_and_move() {
    kfc::BoardModel board = make_board({{"wK", ".", "bK"}, {".", ".", "."}});
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
    assert_token(state, 0, 0, "wK");
    assert_token(state, 1, 1, ".");

    processor.execute("wait 1000", sink);
    assert_token(state, 0, 0, ".");
    assert_token(state, 1, 1, "wK");
}

void test_command_processor_click_outside_grid_ignored() {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 350 50", sink);
    processor.execute("click -10 50", sink);
    assert(!state.has_selection());
}

void test_command_processor_friendly_click_replaces_selection() {
    kfc::BoardModel board = make_board({{"wK", "wN", "bK"}});
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
    assert_token(state, 0, 0, "wK");
    assert_token(state, 0, 1, "wN");
}

void test_command_processor_print_board() {
    kfc::GameState state(make_board({{"wK", ".", "bK"}}));
    kfc::CommandProcessor processor(state);

    std::ostringstream output;
    processor.execute("print board", output);
    assert(output.str() == "wK . bK");
}

void test_knight_legal_l_move() {
    const kfc::BoardModel board = make_board({{".", ".", ".", ".", "."},
                              {".", ".", ".", ".", "."},
                              {".", ".", "wN", ".", "."},
                              {".", ".", ".", ".", "."},
                              {".", ".", ".", ".", "."}});
    assert(kfc::is_legal_move(board, 'N', 2, 2, 4, 3));
    assert(kfc::is_legal_move(board, 'N', 2, 2, 0, 3));
    assert(!kfc::is_legal_move(board, 'N', 2, 2, 2, 4));
}

void test_rook_cannot_move_diagonally() {
    const kfc::BoardModel board = make_board({{".", ".", "."}, {".", "wR", "."}, {".", ".", "."}});
    assert(!kfc::is_legal_move(board, 'R', 1, 1, 2, 2));
    assert(!kfc::is_legal_move(board, 'R', 1, 1, 0, 0));
    assert(kfc::is_legal_move(board, 'R', 1, 1, 1, 0));
    assert(kfc::is_legal_move(board, 'R', 1, 1, 2, 1));
}

void test_king_cannot_move_more_than_one_square() {
    const kfc::BoardModel board = make_board({{".", ".", ".", "."}, {".", "wK", ".", "."}, {".", ".", ".", "."}});
    assert(!kfc::is_legal_move(board, 'K', 1, 1, 1, 3));
    assert(!kfc::is_legal_move(board, 'K', 1, 1, 3, 1));
    assert(!kfc::is_legal_move(board, 'K', 1, 1, 3, 3));
    assert(kfc::is_legal_move(board, 'K', 1, 1, 1, 2));
    assert(kfc::is_legal_move(board, 'K', 1, 1, 2, 2));
}

void test_move_respects_board_boundaries() {
    const kfc::BoardModel board = make_board({{"wN", ".", "wR"}, {".", "wK", "."}});
    assert(!kfc::is_legal_move(board, 'N', 0, 0, -1, 1));
    assert(!kfc::is_legal_move(board, 'R', 0, 2, 0, 5));
    assert(!kfc::is_legal_move(board, 'K', 1, 1, 2, 3));
}

void test_rook_blocked_by_friendly_piece() {
    const kfc::BoardModel board = make_board({{".", ".", ".", "."}, {".", "wR", "wP", "."}});
    assert(!kfc::is_legal_move(board, 'R', 1, 1, 1, 3));
    assert(kfc::is_legal_move(board, 'R', 1, 1, 1, 0));
}

void test_rook_captures_enemy_piece() {
    const kfc::BoardModel board = make_board({{".", ".", ".", "."}, {".", "wR", ".", "bP"}});
    assert(kfc::is_legal_move(board, 'R', 1, 1, 1, 3));
}

void test_knight_jumps_over_pieces() {
    const kfc::BoardModel board = make_board({{".", ".", ".", ".", "."},
                              {".", "wP", "bN", ".", "."},
                              {".", ".", "wN", ".", "."},
                              {".", ".", ".", ".", "."},
                              {".", ".", ".", ".", "."}});
    assert(kfc::is_legal_move(board, 'N', 2, 2, 0, 3));
    assert(kfc::is_legal_move(board, 'N', 2, 2, 4, 1));
}

void test_cannot_capture_own_piece() {
    const kfc::BoardModel board = make_board({{".", "wP", "wR"}});
    assert(!kfc::is_legal_move(board, 'R', 0, 2, 0, 1));
    assert(!kfc::is_legal_move(board, 'N', 0, 2, 0, 1));
}

void test_command_processor_capture() {
    kfc::BoardModel board = make_board({{"wR", ".", "bK"}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    assert(state.has_selection());

    processor.execute("click 250 50", sink);
    assert(!state.has_selection());
    assert_token(state, 0, 0, "wR");
    assert_token(state, 0, 2, "bK");

    processor.execute("wait 2000", sink);
    assert_token(state, 0, 0, ".");
    assert_token(state, 0, 2, "wR");
}

void test_white_pawn_forward_move() {
    const kfc::BoardModel board = make_board({{".", ".", "."}, {".", "wP", "."}, {".", ".", "."}});
    assert(kfc::is_legal_move(board, 'P', 1, 1, 0, 1));
    assert(!kfc::is_legal_move(board, 'P', 1, 1, 0, 0));
    assert(!kfc::is_legal_move(board, 'P', 1, 1, 0, 2));
}

void test_white_pawn_blocked_forward() {
    const kfc::BoardModel board = make_board({{".", "bK", "."}, {".", "wP", "."}});
    assert(!kfc::is_legal_move(board, 'P', 1, 1, 0, 1));
}

void test_white_pawn_diagonal_capture() {
    const kfc::BoardModel board = make_board({{"bN", ".", "bR"}, {".", "wP", "."}, {".", ".", "."}});
    assert(kfc::is_legal_move(board, 'P', 1, 1, 0, 0));
    assert(kfc::is_legal_move(board, 'P', 1, 1, 0, 2));

    const kfc::BoardModel empty_diagonal = make_board({{".", ".", "."}, {".", "wP", "."}});
    assert(!kfc::is_legal_move(empty_diagonal, 'P', 1, 1, 0, 0));
    assert(!kfc::is_legal_move(empty_diagonal, 'P', 1, 1, 0, 2));
}

void test_white_pawn_cannot_move_backward_or_forward_capture() {
    const kfc::BoardModel board = make_board({{".", "wP", "."}, {".", "bK", "."}, {".", ".", "."}});
    assert(!kfc::is_legal_move(board, 'P', 0, 1, 1, 1));
    assert(!kfc::is_legal_move(board, 'P', 0, 1, 1, 0));
    assert(!kfc::is_legal_move(board, 'P', 0, 1, 1, 2));
}

void test_white_pawn_cannot_capture_own_piece() {
    const kfc::BoardModel board = make_board({{"wN", ".", "wR"}, {".", "wP", "."}});
    assert(!kfc::is_legal_move(board, 'P', 1, 1, 0, 0));
    assert(!kfc::is_legal_move(board, 'P', 1, 1, 0, 2));
}

void test_black_pawn_forward_move() {
    const kfc::BoardModel board = make_board({{".", ".", "."}, {".", "bP", "."}, {".", ".", "."}});
    assert(kfc::is_legal_move(board, 'P', 1, 1, 2, 1));
    assert(!kfc::is_legal_move(board, 'P', 1, 1, 2, 0));
    assert(!kfc::is_legal_move(board, 'P', 1, 1, 2, 2));
}

void test_black_pawn_blocked_forward() {
    const kfc::BoardModel board = make_board({{".", "bP", "."}, {".", "wK", "."}});
    assert(!kfc::is_legal_move(board, 'P', 0, 1, 1, 1));
}

void test_black_pawn_diagonal_capture() {
    const kfc::BoardModel board = make_board({{".", ".", "."}, {".", "bP", "."}, {"wR", ".", "wN"}});
    assert(kfc::is_legal_move(board, 'P', 1, 1, 2, 0));
    assert(kfc::is_legal_move(board, 'P', 1, 1, 2, 2));

    const kfc::BoardModel empty_diagonal = make_board({{".", "bP", "."}, {".", ".", "."}});
    assert(!kfc::is_legal_move(empty_diagonal, 'P', 0, 1, 1, 0));
    assert(!kfc::is_legal_move(empty_diagonal, 'P', 0, 1, 1, 2));
}

void test_black_pawn_cannot_move_backward_or_forward_capture() {
    const kfc::BoardModel board = make_board({{".", ".", "."}, {".", "wK", "."}, {".", "bP", "."}});
    assert(!kfc::is_legal_move(board, 'P', 2, 1, 1, 1));
    assert(!kfc::is_legal_move(board, 'P', 2, 1, 1, 0));
    assert(!kfc::is_legal_move(board, 'P', 2, 1, 1, 2));
}

void test_pawn_double_move_from_start_row() {
    const kfc::BoardModel white_board = make_board({{".", ".", "."},
                                    {".", ".", "."},
                                    {".", ".", "."},
                                    {".", "wP", "."}});
    assert(kfc::is_legal_move(white_board, 'P', 3, 1, 1, 1));
    assert(!kfc::is_legal_move(white_board, 'P', 2, 1, 0, 1));

    const kfc::BoardModel black_board = make_board({{".", "bP", "."},
                                    {".", ".", "."},
                                    {".", ".", "."},
                                    {".", ".", "."}});
    assert(kfc::is_legal_move(black_board, 'P', 0, 1, 2, 1));
    assert(!kfc::is_legal_move(black_board, 'P', 1, 1, 3, 1));
}

void test_pawn_double_move_blocked_by_intermediate_piece() {
    const kfc::BoardModel blocked_intermediate = make_board({{".", "bP", "."},
                                             {".", "wN", "."},
                                             {".", ".", "."},
                                             {".", ".", "."}});
    assert(!kfc::is_legal_move(blocked_intermediate, 'P', 0, 1, 2, 1));

    const kfc::BoardModel blocked_dest = make_board({{".", "bP", "."},
                                     {".", ".", "."},
                                     {".", "wR", "."},
                                     {".", ".", "."}});
    assert(!kfc::is_legal_move(blocked_dest, 'P', 0, 1, 2, 1));
}

void test_pawn_promotion_to_queen() {
    kfc::BoardModel white_board = make_board({{".", ".", "."}, {".", "wP", "."}});
    kfc::GameState white_state(white_board);
    white_state.select(1, 1);
    white_state.move_selected_to(0, 1);
    white_state.add_clock(1000);
    assert_token(white_state, 0, 1, "wQ");
    assert_token(white_state, 1, 1, ".");

    kfc::BoardModel black_board = make_board({{".", "bP", "."}, {".", ".", "."}});
    kfc::GameState black_state(black_board);
    black_state.select(0, 1);
    black_state.move_selected_to(1, 1);
    black_state.add_clock(1000);
    assert_token(black_state, 1, 1, "bQ");
    assert_token(black_state, 0, 1, ".");
}

void test_command_processor_rejects_illegal_move() {
    kfc::BoardModel board = make_board({{"wK", ".", ".", "."}, {".", ".", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    assert(state.has_selection());

    processor.execute("click 350 50", sink);
    assert(state.has_selection());
    assert_token(state, 0, 0, "wK");
    assert_token(state, 0, 3, ".");
}

void test_pending_move_print_before_arrival() {
    kfc::BoardModel board = make_board({{"wK", ".", "bK"}, {".", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 150 150", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    assert(output.str() == "wK . bK\n. . .");
}

void test_pending_move_print_after_wait() {
    kfc::BoardModel board = make_board({{"wK", ".", "bK"}, {".", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 150 150", sink);
    processor.execute("wait 1000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    assert(output.str() == ". . bK\n. wK .");
}

void test_two_cell_move_before_and_after_arrival() {
    kfc::BoardModel board = make_board({{"wR", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    processor.execute("wait 1000", sink);

    std::ostringstream first_print;
    processor.execute("print board", first_print);
    assert(first_print.str() == "wR . .");

    processor.execute("wait 1000", sink);

    std::ostringstream second_print;
    processor.execute("print board", second_print);
    assert(second_print.str() == ". . wR");
}

void test_is_piece_moving_while_in_transit() {
    kfc::GameState state(make_board({{"wR", ".", "."}}));
    state.select(0, 0);
    state.move_selected_to(0, 2);
    assert(state.is_piece_moving(0, 0));
    assert(!state.is_piece_moving(0, 1));
    assert(!state.is_piece_moving(0, 2));
}

void test_is_piece_moving_false_after_settle_no_cooldown() {
    kfc::GameState state(make_board({{"wR", ".", "."}}));
    state.select(0, 0);
    state.move_selected_to(0, 2);
    assert(state.is_piece_moving(0, 0));

    state.add_clock(2000);
    assert(!state.is_piece_moving(0, 0));
    assert(!state.is_piece_moving(0, 2));
    assert(state.is_selectable_piece(0, 2));
}

void test_click_on_moving_piece_does_not_select_or_redirect() {
    kfc::BoardModel board = make_board({{"wR", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    processor.execute("wait 1000", sink);

    processor.execute("click 50 50", sink);
    assert(!state.has_selection());

    processor.execute("click 150 50", sink);
    processor.execute("wait 1000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    assert(output.str() == ". . wR");
}

void test_piece_can_move_immediately_after_settle() {
    kfc::BoardModel board = make_board({{"wR", ".", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 150 50", sink);
    processor.execute("wait 1000", sink);
    assert(!state.is_piece_moving(0, 1));

    processor.execute("click 150 50", sink);
    assert(state.has_selection());

    processor.execute("click 250 50", sink);
    processor.execute("wait 1000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    assert(output.str() == ". . wR .");
}

void test_opposite_colors_do_not_move_concurrently_in_common_route() {
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
    assert(output.str() == ". . wR\n. . .\nbR . .");
}

void test_opposite_colors_can_move_on_disjoint_routes() {
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
    assert(output.str() == ". wR . .\n. . . .\n. . . bR");
}

void test_moving_piece_ignores_redirect() {
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
    assert(output.str() == ". . wR");
}

void test_move_aborted_if_friendly_occupies_target_before_arrival() {
    kfc::BoardModel board = make_board({{"wR", ".", ".", "."}, {".", ".", ".", "."}});
    kfc::GameState state(board);
    state.select(0, 0);
    state.move_selected_to(0, 2);

    state.set_piece(0, 2, *kfc::Piece::from_token("wK"));

    state.add_clock(2000);

    assert_token(state, 0, 0, "wR");
    assert_token(state, 0, 2, "wK");
}

void test_rejects_move_for_piece_already_in_pending_move() {
    kfc::BoardModel board = make_board({{"wR", ".", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    assert(state.is_piece_moving(0, 0));

    processor.execute("click 50 50", sink);
    assert(!state.has_selection());

    processor.execute("click 150 50", sink);
    assert(!state.has_selection());

    processor.execute("wait 1000", sink);
    assert(state.is_piece_moving(0, 0));

    processor.execute("wait 1000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    assert(output.str() == ". . wR");
}

void test_rejects_two_same_color_moves_to_same_square() {
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
    assert(output.str() == ". . wR .\n. . . .\n. . . wN");
}

void test_king_capture_sets_game_over() {
    kfc::BoardModel board = make_board({{"wR", ".", "bK"}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    assert(!state.is_game_over());

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    assert(!state.is_game_over());

    processor.execute("wait 2000", sink);
    assert(state.is_game_over());
    assert_token(state, 0, 0, ".");
    assert_token(state, 0, 2, "wR");
}

void test_commands_ignored_after_game_over() {
    kfc::BoardModel board = make_board({{"wR", ".", "bK"}, {".", "wN", "."}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    processor.execute("wait 2000", sink);
    assert(state.is_game_over());

    const kfc::BoardModel board_snapshot = state.board();
    const std::int64_t clock_snapshot = state.clock_ms();

    processor.execute("click 150 150", sink);
    assert(!state.has_selection());

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    assert(!state.has_selection());
    assert(state.board() == board_snapshot);

    processor.execute("wait 5000", sink);
    assert(state.clock_ms() == clock_snapshot);
    assert(state.board() == board_snapshot);
}

void test_print_board_after_king_capture() {
    kfc::BoardModel board = make_board({{"wR", ".", "bK"}});
    kfc::GameState state(board);
    kfc::CommandProcessor processor(state);
    std::ostringstream sink;

    processor.execute("click 50 50", sink);
    processor.execute("click 250 50", sink);
    processor.execute("wait 2000", sink);

    std::ostringstream output;
    processor.execute("print board", output);
    assert(output.str() == ". . wR");
}

void test_jump_capture_intercepts_arriving_enemy() {
    kfc::BoardModel board = make_board({{".", ".", "."}, {"wR", ".", "."}, {"bR", ".", "."}});
    kfc::GameState state(board);

    state.select(2, 0);
    state.move_selected_to(1, 0);
    state.add_clock(500);

    state.select(1, 0);
    state.jump_selected();
    assert(state.is_piece_jumping(1, 0));

    state.add_clock(500);

    assert_token(state, 2, 0, ".");
    assert_token(state, 1, 0, "wR");
}

void test_moving_piece_cannot_jump() {
    kfc::GameState state(make_board({{"wR", ".", "."}}));
    state.select(0, 0);
    state.move_selected_to(0, 2);
    assert(state.is_piece_moving(0, 0));

    state.select(0, 0);
    state.jump_selected();
    assert(state.is_piece_jumping(0, 0) == false);

    state.add_clock(2000);
    assert_token(state, 0, 2, "wR");
}

void test_jump_status_cleared_after_duration() {
    kfc::GameState state(make_board({{"wR", ".", "."}}));
    state.select(0, 0);
    state.jump_selected();
    assert(state.is_piece_jumping(0, 0));

    state.add_clock(1000);
    assert(!state.is_piece_jumping(0, 0));
    assert_token(state, 0, 0, "wR");
}

void test_jump_command_airborne_piece_captures_arriving_enemy() {
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
    assert(output.str() == ". . .\nwK . .\n. . .");
}

void test_jump_command_too_late_does_not_save_piece() {
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
    assert(output.str() == ". . .\nbR . .\n. . .");
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
    test_white_pawn_forward_move();
    test_white_pawn_blocked_forward();
    test_white_pawn_diagonal_capture();
    test_white_pawn_cannot_move_backward_or_forward_capture();
    test_white_pawn_cannot_capture_own_piece();
    test_black_pawn_forward_move();
    test_black_pawn_blocked_forward();
    test_black_pawn_diagonal_capture();
    test_black_pawn_cannot_move_backward_or_forward_capture();
    test_pawn_double_move_from_start_row();
    test_pawn_double_move_blocked_by_intermediate_piece();
    test_pawn_promotion_to_queen();
    test_command_processor_capture();
    test_command_processor_rejects_illegal_move();
    test_pending_move_print_before_arrival();
    test_pending_move_print_after_wait();
    test_two_cell_move_before_and_after_arrival();
    test_is_piece_moving_while_in_transit();
    test_is_piece_moving_false_after_settle_no_cooldown();
    test_click_on_moving_piece_does_not_select_or_redirect();
    test_piece_can_move_immediately_after_settle();
    test_opposite_colors_do_not_move_concurrently_in_common_route();
    test_opposite_colors_can_move_on_disjoint_routes();
    test_moving_piece_ignores_redirect();
    test_move_aborted_if_friendly_occupies_target_before_arrival();
    test_rejects_move_for_piece_already_in_pending_move();
    test_rejects_two_same_color_moves_to_same_square();
    test_king_capture_sets_game_over();
    test_commands_ignored_after_game_over();
    test_print_board_after_king_capture();
    test_jump_capture_intercepts_arriving_enemy();
    test_moving_piece_cannot_jump();
    test_jump_status_cleared_after_duration();
    test_jump_command_airborne_piece_captures_arriving_enemy();
    test_jump_command_too_late_does_not_save_piece();
    return 0;
}
