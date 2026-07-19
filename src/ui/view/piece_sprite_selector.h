#pragma once

#include "ui/assets/sprite_animation_constants.h"
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
    [[nodiscard]] static int frame_from_progress(float progress);
};

}  // namespace kfc
