#include "ui/layout/board_layout_calculator.h"

#include <algorithm>

namespace kfc {

BoardLayout BoardLayoutCalculator::calculate(int window_width, int window_height,
                                             int board_width_cells, int board_height_cells) const {
    if (window_width <= 0 || window_height <= 0 || board_width_cells <= 0 ||
        board_height_cells <= 0) {
        return {};
    }

    const int cell_size = std::min(window_width / board_width_cells,
                                   window_height / board_height_cells);
    const int board_width_pixels = cell_size * board_width_cells;
    const int board_height_pixels = cell_size * board_height_cells;
    const int board_x = (window_width - board_width_pixels) / 2;
    const int board_y = (window_height - board_height_pixels) / 2;

    return BoardLayout{board_x, board_y, cell_size, board_width_pixels, board_height_pixels};
}

}  // namespace kfc
