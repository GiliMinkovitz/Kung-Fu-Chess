#include "command_processor.h"

#include "board.h"
#include "move_validator.h"

#include <sstream>
#include <string>

namespace kfc {

CommandProcessor::CommandProcessor(GameState& state) : state_(state) {}

void CommandProcessor::execute(const std::string& command, std::ostream& out) {
    std::istringstream stream(command);
    std::string verb;
    stream >> verb;

    if (verb == "click") {
        int x = 0;
        int y = 0;
        if (stream >> x >> y) {
            handle_click(x, y);
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

    if (verb == "print" && command == "print board") {
        handle_print_board(out);
    }
}

void CommandProcessor::handle_click(int x, int y) {
    std::size_t row = 0;
    std::size_t col = 0;
    if (!pixel_to_cell(x, y, row, col) || !state_.is_in_bounds(row, col)) {
        return;
    }

    if (!state_.has_selection()) {
        if (state_.is_selectable_piece(row, col)) {
            state_.select(row, col);
        }
        return;
    }

    if (state_.is_friendly_to_selection(row, col)) {
        state_.select(row, col);
        return;
    }

    std::size_t from_row = 0;
    std::size_t from_col = 0;
    if (!state_.selection(from_row, from_col)) {
        return;
    }

    if (state_.is_piece_moving(from_row, from_col)) {
        return;
    }

    const std::string& moving_token = state_.board()[from_row][from_col];
    const char piece = moving_token[1];
    const char color = moving_token[0];
    if (!is_legal_move(state_.board(), piece, static_cast<int>(from_row),
                       static_cast<int>(from_col), static_cast<int>(row), static_cast<int>(col))) {
        return;
    }

    if (state_.is_square_claimed_by_same_color_pending_move(row, col, color)) {
        return;
    }

    state_.move_selected_to(row, col);
}

void CommandProcessor::handle_wait(std::int64_t ms) {
    state_.add_clock(ms);
}

void CommandProcessor::handle_print_board(std::ostream& out) {
    state_.settle_pending_moves();
    write_board(out, state_.board());
}

bool CommandProcessor::pixel_to_cell(int x, int y, std::size_t& row, std::size_t& col) const {
    if (x < 0 || y < 0) {
        return false;
    }

    const Board& board = state_.board();
    if (board.empty() || board.front().empty()) {
        return false;
    }

    col = static_cast<std::size_t>(x) / static_cast<std::size_t>(kCellPixelSize);
    row = static_cast<std::size_t>(y) / static_cast<std::size_t>(kCellPixelSize);
    return col < board.front().size() && row < board.size();
}

}  // namespace kfc
