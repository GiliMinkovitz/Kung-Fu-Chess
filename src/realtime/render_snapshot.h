#pragma once

#include "../model/piece.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace kfc {

struct ActiveMoveSnapshot {
    Piece::Id piece_id = Piece::kInvalidId;
    std::size_t from_row = 0;
    std::size_t from_col = 0;
    std::size_t to_row = 0;
    std::size_t to_col = 0;
    float progress = 0.0f;
};

struct ActiveJumpSnapshot {
    Piece::Id piece_id = Piece::kInvalidId;
    std::size_t row = 0;
    std::size_t col = 0;
    float progress = 0.0f;
};

struct AnimationSnapshot {
    std::vector<ActiveMoveSnapshot> moves;
    std::vector<ActiveJumpSnapshot> jumps;
};

[[nodiscard]] inline float compute_animation_progress(std::int64_t clock_ms,
                                                      std::int64_t start_time,
                                                      std::int64_t arrival_time) noexcept {
    if (clock_ms >= arrival_time) {
        return 1.0f;
    }
    if (clock_ms <= start_time) {
        return 0.0f;
    }

    const std::int64_t duration = arrival_time - start_time;
    if (duration <= 0) {
        return 1.0f;
    }

    return static_cast<float>(clock_ms - start_time) / static_cast<float>(duration);
}

}  // namespace kfc
