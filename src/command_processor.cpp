#include "command_processor.h"

#include "board.h"

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
        if (state_.is_piece(row, col)) {
            state_.select(row, col);
        }
        return;
    }

    if (state_.is_friendly_to_selection(row, col)) {
        state_.select(row, col);
    } else {
        state_.move_selected_to(row, col);
    }
}

void CommandProcessor::handle_wait(std::int64_t ms) {
    state_.add_clock(ms);
}

void CommandProcessor::handle_print_board(std::ostream& out) const {
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
