#include "adapters/board_writer.h"

#include <ostream>

namespace kfc {

void write_board(std::ostream& out, const BoardModel& board) {
    for (std::size_t row = 0; row < board.rows(); ++row) {
        for (std::size_t col = 0; col < board.cols(); ++col) {
            if (col > 0) {
                out << ' ';
            }
            out << board.token_at(row, col);
        }
        if (row + 1 < board.rows()) {
            out << '\n';
        }
    }
}

}  // namespace kfc
