#pragma once

#include <iosfwd>

namespace kfc {

struct Position {
    int row = 0;
    int col = 0;
};

[[nodiscard]] bool operator==(const Position& lhs, const Position& rhs) noexcept;
[[nodiscard]] bool operator!=(const Position& lhs, const Position& rhs) noexcept;

std::ostream& operator<<(std::ostream& out, const Position& pos);

}  // namespace kfc
