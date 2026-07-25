#include "ui/view/piece_sprite_selector.h"

#include <algorithm>

namespace kfc {

int PieceSpriteSelector::frame_from_progress(float progress) {
    const float clamped = std::clamp(progress, 0.0f, 1.0f);
    const int frame_index = std::min(static_cast<int>(clamped * kSpriteAnimationFrameCount),
                                     kSpriteAnimationFrameCount - 1);
    return frame_index + 1;
}

PieceSpriteSelection PieceSpriteSelector::select(const PieceSpriteContext& context) const {
    if (context.moving) {
        return {kSpriteStateMove, frame_from_progress(context.progress)};
    }
    if (context.jumping) {
        return {kSpriteStateJump, frame_from_progress(context.progress)};
    }
    if (context.resting) {
        const std::string_view state =
            context.rest_kind == RestKind::Long ? kSpriteStateLongRest : kSpriteStateShortRest;
        return {state, frame_from_progress(context.progress)};
    }
    return {kSpriteStateIdle, kSpriteIdleFrame};
}

}  // namespace kfc
