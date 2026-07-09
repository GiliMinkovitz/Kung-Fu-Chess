#pragma once

#include "core/piece.h"

#include <optional>
#include <string>
#include <utility>

namespace kfc {

struct PieceDescriptor {
    PieceColor color;
    PieceKind kind;
};

[[nodiscard]] bool is_empty_token(const std::string& token) noexcept;
[[nodiscard]] bool is_valid_token(const std::string& token) noexcept;
[[nodiscard]] std::optional<PieceDescriptor> descriptor_from_token(const std::string& token);
[[nodiscard]] std::string to_token(PieceColor color, PieceKind kind);
[[nodiscard]] char color_to_char(PieceColor color) noexcept;
[[nodiscard]] char kind_to_char(PieceKind kind) noexcept;
[[nodiscard]] std::optional<PieceColor> color_from_char(char color) noexcept;
[[nodiscard]] std::optional<PieceKind> kind_from_char(char kind) noexcept;

}  // namespace kfc
