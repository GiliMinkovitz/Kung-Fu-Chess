#include "core/piece.h"

namespace kfc {

bool Piece::is_white() const noexcept {
    return color == PieceColor::White;
}

bool Piece::is_black() const noexcept {
    return color == PieceColor::Black;
}

bool Piece::is_same_color_as(const Piece& other) const noexcept {
    return id != kInvalidId && other.id != kInvalidId && color == other.color;
}

bool Piece::is_opponent_of(const Piece& other) const noexcept {
    return id != kInvalidId && other.id != kInvalidId && color != other.color;
}

bool operator==(const Piece& lhs, const Piece& rhs) noexcept {
    return lhs.id == rhs.id;
}

bool operator!=(const Piece& lhs, const Piece& rhs) noexcept {
    return !(lhs == rhs);
}

}  // namespace kfc
