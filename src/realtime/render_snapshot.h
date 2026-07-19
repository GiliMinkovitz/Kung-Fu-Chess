#pragma once

#include "../model/piece.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace kfc {

enum class RestKind { Short, Long };

struct ActiveMoveSnapshot {
    Piece::Id piece_id = Piece::kInvalidId;
    PieceKind kind = PieceKind::Pawn;
    PieceColor color = PieceColor::White;
    std::size_t from_row = 0;
    std::size_t from_col = 0;
    std::size_t to_row = 0;
    std::size_t to_col = 0;
    float progress = 0.0f;
};

struct ActiveJumpSnapshot {
    Piece::Id piece_id = Piece::kInvalidId;
    PieceKind kind = PieceKind::Pawn;
    PieceColor color = PieceColor::White;
    std::size_t row = 0;
    std::size_t col = 0;
    float progress = 0.0f;
};

struct ActiveRestSnapshot {
    Piece::Id piece_id = Piece::kInvalidId;
    std::size_t row = 0;
    std::size_t col = 0;
    RestKind kind = RestKind::Short;
    float progress = 0.0f;
};

struct AnimationSnapshot {
    // Active move/jump/rest animations. In-flight pieces are drawn from moves/jumps only;
    // the board grid may have an empty cell while a move or jump entry is present.
    std::vector<ActiveMoveSnapshot> moves;
    std::vector<ActiveJumpSnapshot> jumps;
    std::vector<ActiveRestSnapshot> rests;
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
    return static_cast<float>(clock_ms - start_time) / static_cast<float>(duration);
}

}  // namespace kfc
