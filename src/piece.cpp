#include "piece.h"

#include "game_config.h"

namespace kfc {

Piece Piece::empty() noexcept {
    return {};
}

bool Piece::is_white() const noexcept {
    return color == kWhiteColor;
}

bool Piece::is_black() const noexcept {
    return color == kBlackColor;
}

bool Piece::is_same_color_as(const Piece& other) const noexcept {
    return !is_empty() && !other.is_empty() && color == other.color;
}

bool Piece::is_opponent_of(const Piece& other) const noexcept {
    return !is_empty() && !other.is_empty() && color != other.color;
}

bool is_valid_token(const std::string& token) noexcept {
    if (token.size() == 1 && token[0] == kEmptyToken) {
        return true;
    }
    if (token.size() != 2) {
        return false;
    }

    const char color = token[0];
    const char piece = token[1];
    if (color != kWhiteColor && color != kBlackColor) {
        return false;
    }

    switch (piece) {
        case kKingType:
        case kQueenType:
        case kRookType:
        case kBishopType:
        case kKnightType:
        case kPawnType:
            return true;
        default:
            return false;
    }
}

std::optional<Piece> Piece::from_token(const std::string& token) {
    if (token.size() == 1 && token[0] == kEmptyToken) {
        return empty();
    }
    if (!is_valid_token(token)) {
        return std::nullopt;
    }
    return Piece{token[0], token[1]};
}

std::string Piece::to_token() const {
    if (is_empty()) {
        return std::string{kEmptyToken};
    }
    return std::string{color, type};
}

bool operator==(const Piece& lhs, const Piece& rhs) noexcept {
    return lhs.color == rhs.color && lhs.type == rhs.type;
}

bool operator!=(const Piece& lhs, const Piece& rhs) noexcept {
    return !(lhs == rhs);
}

}  // namespace kfc
