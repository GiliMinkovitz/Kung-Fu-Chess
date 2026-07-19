#pragma once

#include <string_view>

namespace kfc {

constexpr int kSpriteAnimationFrameCount = 5;
constexpr int kSpriteIdleFrame = 1;

constexpr std::string_view kSpriteStateIdle = "idle";
constexpr std::string_view kSpriteStateMove = "move";
constexpr std::string_view kSpriteStateJump = "jump";
constexpr std::string_view kSpriteStateShortRest = "short_rest";
constexpr std::string_view kSpriteStateLongRest = "long_rest";

}  // namespace kfc
