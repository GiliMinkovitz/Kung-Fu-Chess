#pragma once

#include <optional>
#include <string>

namespace kfc {

struct Piece {
    char color = '\0';
    char type = '\0';

    [[nodiscard]] static std::optional<Piece> from_token(const std::string& token);
    [[nodiscard]] std::string to_token() const;
    [[nodiscard]] bool is_empty() const noexcept { return type == '\0'; }
    [[nodiscard]] static Piece empty() noexcept;
};

[[nodiscard]] bool is_valid_token(const std::string& token) noexcept;
[[nodiscard]] bool operator==(const Piece& lhs, const Piece& rhs) noexcept;
[[nodiscard]] bool operator!=(const Piece& lhs, const Piece& rhs) noexcept;

}  // namespace kfc
