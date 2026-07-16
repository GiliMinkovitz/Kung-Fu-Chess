#pragma once

#include "ui/view/board_view_model.h"

namespace kfc {

struct PieceSpriteContext {
    PieceView piece;
    bool moving = false;
    bool jumping = false;
    bool resting = false;
    RestKind rest_kind = RestKind::Short;
    float progress = 0.0f;
};

}  // namespace kfc
