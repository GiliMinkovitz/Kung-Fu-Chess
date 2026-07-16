#pragma once

#include "ui/view/piece_sprite_context.h"

#include <string_view>

namespace kfc {

struct PieceSpriteSelection {
    std::string_view state;
    int frame = 1;
};

class PieceSpriteSelector {
public:
    [[nodiscard]] PieceSpriteSelection select(const PieceSpriteContext& context) const;

private:
    static constexpr int kMoveFrameCount = 5;
    [[nodiscard]] static int frame_from_progress(float progress);
};

}  // namespace kfc
