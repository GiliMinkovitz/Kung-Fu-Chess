#pragma once

#include "ui/layout/board_layout.h"

namespace kfc {

class BoardLayoutCalculator {
public:
    [[nodiscard]] BoardLayout calculate(int window_width, int window_height, int board_width_cells,
                                        int board_height_cells) const;
};

}  // namespace kfc
