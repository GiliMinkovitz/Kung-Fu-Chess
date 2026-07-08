#include "piece.h"

namespace kfc {

Piece Piece::empty() noexcept {
    return {};
}

bool is_valid_token(const std::string& token) noexcept {
    if (token == ".") {
        return true;
    }
    if (token.size() != 2) {
        return false;
    }

    const char color = token[0];
    const char piece = token[1];
    if (color != 'w' && color != 'b') {
        return false;
    }

    switch (piece) {
        case 'K':
        case 'Q':
        case 'R':
        case 'B':
        case 'N':
        case 'P':
            return true;
        default:
            return false;
    }
}

std::optional<Piece> Piece::from_token(const std::string& token) {
    if (token == ".") {
        return empty();
    }
    if (!is_valid_token(token)) {
        return std::nullopt;
    }
    return Piece{token[0], token[1]};
}

std::string Piece::to_token() const {
    if (is_empty()) {
        return ".";
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
