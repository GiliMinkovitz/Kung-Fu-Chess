#include "ui/view/piece_sprite_selector.h"

#include <algorithm>

namespace kfc {

int PieceSpriteSelector::frame_from_progress(float progress) {
    const float clamped = std::clamp(progress, 0.0f, 1.0f);
    const int frame_index =
        std::min(static_cast<int>(clamped * kMoveFrameCount), kMoveFrameCount - 1);
    return frame_index + 1;
}

PieceSpriteSelection PieceSpriteSelector::select(const PieceSpriteContext& context) const {
    if (context.moving) {
        return {"move", frame_from_progress(context.progress)};
    }
    return {"idle", 1};
}

}  // namespace kfc
