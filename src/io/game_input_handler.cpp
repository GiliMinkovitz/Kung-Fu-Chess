#include "game_input_handler.h"

#include "board_writer.h"
#include "../model/game_config.h"

#include <sstream>
#include <string>

namespace kfc {

GameInputHandler::GameInputHandler(GameState& state, int cell_pixel_size)
    : state_(state), cell_pixel_size_(cell_pixel_size) {}

void GameInputHandler::execute(const std::string& command, std::ostream& out) {
    std::istringstream stream(command);
    std::string verb;
    stream >> verb;

    if (verb == "click") {
        int x = 0;
        int y = 0;
        if (stream >> x >> y) {
            handle_pixel_click(x, y);
        }
        return;
    }

    if (verb == "jump") {
        int x = 0;
        int y = 0;
        if (stream >> x >> y) {
            handle_pixel_jump(x, y);
        }
        return;
    }

    if (verb == "wait") {
        std::int64_t ms = 0;
        if (stream >> ms) {
            handle_wait(ms);
        }
        return;
    }

    if (verb == "print" && command == kPrintBoardCommand) {
        handle_print_board(out);
        out << '\n';
    }
}

void GameInputHandler::handle_pixel_click(int x, int y) {
    handle_click(x, y);
}

void GameInputHandler::handle_pixel_jump(int x, int y) {
    handle_jump(x, y);
}

// Click state machine: no selection -> select; friendly -> re-select or jump; else move.
void GameInputHandler::handle_click(int x, int y) {
    if (state_.is_game_over()) {
        return;
    }

    std::size_t row = 0;
    std::size_t col = 0;
    if (!pixel_to_cell(x, y, row, col) || !state_.is_in_bounds(row, col)) {
        return;
    }

    if (!state_.has_selection()) {
        handle_select(row, col);
        return;
    }

    if (state_.is_friendly_to_selection(row, col)) {
        handle_friendly_click(row, col);
        return;
    }

    handle_move_attempt(row, col);
}

void GameInputHandler::handle_select(std::size_t row, std::size_t col) {
    if (state_.is_selectable_piece(row, col)) {
        state_.select(row, col);
    }
}

void GameInputHandler::handle_friendly_click(std::size_t row, std::size_t col) {
    std::size_t from_row = 0;
    std::size_t from_col = 0;
    // Clicking the already-selected friendly piece triggers a jump, not a re-select.
    if (state_.selection(from_row, from_col) && from_row == row && from_col == col) {
        if (!state_.is_piece_moving(from_row, from_col) &&
            !state_.is_piece_jumping(from_row, from_col)) {
            state_.jump_selected();
        }
        return;
    }

    state_.select(row, col);
}

void GameInputHandler::handle_move_attempt(std::size_t row, std::size_t col) {
    std::size_t from_row = 0;
    std::size_t from_col = 0;
    state_.selection(from_row, from_col);

    if (state_.is_piece_moving(from_row, from_col) || state_.is_piece_jumping(from_row, from_col)) {
        return;
    }

    state_.move_selected_to(row, col);
}

void GameInputHandler::handle_jump(int x, int y) {
    if (state_.is_game_over()) {
        return;
    }

    std::size_t row = 0;
    std::size_t col = 0;
    if (!pixel_to_cell(x, y, row, col) || !state_.is_in_bounds(row, col)) {
        return;
    }

    if (state_.is_piece_moving(row, col) || state_.is_piece_jumping(row, col)) {
        return;
    }

    state_.jump_at(row, col);
    state_.clear_selection();
}

void GameInputHandler::handle_wait(std::int64_t ms) {
    if (state_.is_game_over()) {
        return;
    }

    state_.add_clock(ms);
}

void GameInputHandler::handle_print_board(std::ostream& out) {
    state_.write_board(out, kfc::write_board);
}

bool GameInputHandler::pixel_to_cell(int x, int y, std::size_t& row, std::size_t& col) const {
    if (x < 0 || y < 0) {
        return false;
    }

    if (state_.rows() == 0 || state_.cols() == 0) {
        return false;
    }

    col = static_cast<std::size_t>(x) / static_cast<std::size_t>(cell_pixel_size_);
    row = static_cast<std::size_t>(y) / static_cast<std::size_t>(cell_pixel_size_);
    return col < state_.cols() && row < state_.rows();
}

}  // namespace kfc
