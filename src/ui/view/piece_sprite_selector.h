#pragma once

#include "ui/view/board_view_model.h"

#include <string_view>

namespace kfc {

struct PieceSpriteSelection {
    std::string_view state;
    int frame = 1;
};

class PieceSpriteSelector {
public:
    [[nodiscard]] PieceSpriteSelection select(const PieceView& piece) const;
};

}  // namespace kfc
