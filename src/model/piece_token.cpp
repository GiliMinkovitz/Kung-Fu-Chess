#include "piece_token.h"

#include "game_config.h"

#include <algorithm>
#include <array>
#include <cstddef>

namespace kfc {

namespace {

struct PieceKindMapping {
    PieceKind kind;
    char character;
};

constexpr std::size_t kPlayablePieceKindCount = static_cast<std::size_t>(PieceKind::Count);

constexpr std::array<PieceKindMapping, kPlayablePieceKindCount> kPieceKindMappings = {{
    {PieceKind::King, kKingType},
    {PieceKind::Queen, kQueenType},
    {PieceKind::Rook, kRookType},
    {PieceKind::Bishop, kBishopType},
    {PieceKind::Knight, kKnightType},
    {PieceKind::Pawn, kPawnType},
}};

}  // namespace

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
    const auto it = std::find_if(kPieceKindMappings.begin(), kPieceKindMappings.end(),
                                 [kind](const PieceKindMapping& entry) { return entry.kind == kind; });
    if (it == kPieceKindMappings.end()) {
        return kPawnType;
    }
    return it->character;
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

std::optional<PieceKind> kind_from_char(char character) noexcept {
    const auto it = std::find_if(kPieceKindMappings.begin(), kPieceKindMappings.end(),
                                 [character](const PieceKindMapping& entry) {
                                     return entry.character == character;
                                 });
    if (it == kPieceKindMappings.end()) {
        return std::nullopt;
    }
    return it->kind;
}

}  // namespace kfc
