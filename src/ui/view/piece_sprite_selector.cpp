#include "ui/view/piece_sprite_selector.h"

#include <algorithm>
#include <exception>
#include <iostream>
#include <typeinfo>

namespace kfc {

namespace {

const char* diag_rest_kind_name(RestKind rest_kind) {
    return rest_kind == RestKind::Long ? "Long" : "Short";
}

}  // namespace

int PieceSpriteSelector::frame_from_progress(float progress) {
    const float clamped = std::clamp(progress, 0.0f, 1.0f);
    const int frame_index = std::min(static_cast<int>(clamped * kSpriteAnimationFrameCount),
                                     kSpriteAnimationFrameCount - 1);
    return frame_index + 1;
}

PieceSpriteSelection PieceSpriteSelector::select(const PieceSpriteContext& context) const {
    try {
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
    } catch (const std::exception& ex) {
        std::cerr << "[REST-RENDER-DIAG] EXCEPTION"
                  << " function=PieceSpriteSelector::select"
                  << " type=" << typeid(ex).name() << " what=" << ex.what()
                  << " rest_kind=" << diag_rest_kind_name(context.rest_kind)
                  << " progress=" << context.progress << '\n';
        throw;
    }
}

}  // namespace kfc
