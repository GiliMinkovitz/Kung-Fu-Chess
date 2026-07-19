#pragma once

#include "position.h"

#include <cstdint>

namespace kfc {

enum class PieceColor { White, Black };

enum class PieceKind { King, Queen, Rook, Bishop, Knight, Pawn, Count };

enum class PieceState {
    Idle,     // On the board grid at piece.cell.
    Moving,   // In transit: may be off-grid; PendingMove + AnimationSnapshot track the move.
    Captured  // Removed from play.
};

class Piece {
public:
    using Id = std::uint32_t;
    static constexpr Id kInvalidId = 0;

    Id id = kInvalidId;
    PieceColor color = PieceColor::White;
    PieceKind kind = PieceKind::Pawn;
    Position cell{};
    PieceState state = PieceState::Idle;

    [[nodiscard]] bool is_white() const noexcept;
    [[nodiscard]] bool is_black() const noexcept;
    [[nodiscard]] bool is_same_color_as(const Piece& other) const noexcept;
    [[nodiscard]] bool is_opponent_of(const Piece& other) const noexcept;
};

[[nodiscard]] bool operator==(const Piece& lhs, const Piece& rhs) noexcept;
[[nodiscard]] bool operator!=(const Piece& lhs, const Piece& rhs) noexcept;

}  // namespace kfc
