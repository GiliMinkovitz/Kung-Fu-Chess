#include "bishop_rules.h"

#include "../../logic/path_utils.h"

#include <cstdlib>

namespace kfc::piece_rules {

namespace {

[[nodiscard]] bool is_path_clear(const BoardModel& board, int start_row, int start_col,
                                 int end_row, int end_col) {
    bool clear = true;
    for_each_cell_on_path(start_row, start_col, end_row, end_col,
                          [&](int row, int col) {
                              if (!board.is_empty(static_cast<std::size_t>(row),
                                                  static_cast<std::size_t>(col))) {
                                  clear = false;
                              }
                          });
    return clear;
}

}  // namespace

bool is_bishop_move(const BoardModel& board, int start_row, int start_col, int end_row,
                    int end_col) {
    const int dr = end_row - start_row;
    const int dc = end_col - start_col;
    if (std::abs(dr) != std::abs(dc)) {
        return false;
    }
    return is_path_clear(board, start_row, start_col, end_row, end_col);
}

}  // namespace kfc::piece_rules
