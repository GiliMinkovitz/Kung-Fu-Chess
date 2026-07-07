#include "board.h"

#include <istream>
#include <ostream>
#include <string>

namespace kfc {

bool is_valid_board(const Board& board) noexcept {
    if (board.empty()) {
        return false;
    }

    const std::size_t width = board.front().size();
    if (width == 0) {
        return false;
    }

    for (const std::string& row : board) {
        if (row.size() != width) {
            return false;
        }
    }

    return true;
}

void read_board(std::istream& in, Board& board) {
    board.clear();

    std::string line;
    while (std::getline(in, line)) {
        board.push_back(line);
    }
}

void write_board(std::ostream& out, const Board& board) {
    for (std::size_t row = 0; row < board.size(); ++row) {
        out << board[row];
        if (row + 1 < board.size()) {
            out << '\n';
        }
    }
    if (!board.empty()) {
        out << '\n';
    }
}

}  // namespace kfc
