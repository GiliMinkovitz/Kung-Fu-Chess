#include "piece_token.h"

#include "game_config.h"

namespace kfc {

bool is_empty_token(const std::string& token) noexcept {
    return token.size() == 1 && token[0] == kEmptyToken;
}

bool is_valid_token(const std::string& token) noexcept {
    if (is_empty_token(token)) {
        return true;
    }
    if (token.size() != 2) {
        return false;
    }

    if (!color_from_char(token[0]).has_value()) {
        return false;
    }
    return kind_from_char(token[1]).has_value();
}

std::optional<PieceDescriptor> descriptor_from_token(const std::string& token) {
    if (is_empty_token(token)) {
        return std::nullopt;
    }
    if (!is_valid_token(token)) {
        return std::nullopt;
    }

    const std::optional<PieceColor> color = color_from_char(token[0]);
    const std::optional<PieceKind> kind = kind_from_char(token[1]);
    if (!color.has_value() || !kind.has_value()) {
        return std::nullopt;
    }
    return PieceDescriptor{*color, *kind};
}

std::string to_token(PieceColor color, PieceKind kind) {
    return std::string{color_to_char(color), kind_to_char(kind)};
}

char color_to_char(PieceColor color) noexcept {
    switch (color) {
        case PieceColor::White:
            return kWhiteColor;
        case PieceColor::Black:
            return kBlackColor;
    }
    return kWhiteColor;
}

char kind_to_char(PieceKind kind) noexcept {
    switch (kind) {
        case PieceKind::King:
            return kKingType;
        case PieceKind::Queen:
            return kQueenType;
        case PieceKind::Rook:
            return kRookType;
        case PieceKind::Bishop:
            return kBishopType;
        case PieceKind::Knight:
            return kKnightType;
        case PieceKind::Pawn:
            return kPawnType;
    }
    return kPawnType;
}

std::optional<PieceColor> color_from_char(char color) noexcept {
    if (color == kWhiteColor) {
        return PieceColor::White;
    }
    if (color == kBlackColor) {
        return PieceColor::Black;
    }
    return std::nullopt;
}

std::optional<PieceKind> kind_from_char(char kind) noexcept {
    switch (kind) {
        case kKingType:
            return PieceKind::King;
        case kQueenType:
            return PieceKind::Queen;
        case kRookType:
            return PieceKind::Rook;
        case kBishopType:
            return PieceKind::Bishop;
        case kKnightType:
            return PieceKind::Knight;
        case kPawnType:
            return PieceKind::Pawn;
        default:
            return std::nullopt;
    }
}

}  // namespace kfc
