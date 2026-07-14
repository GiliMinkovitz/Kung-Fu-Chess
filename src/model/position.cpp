#include "position.h"

#include <ostream>

namespace kfc {

bool operator==(const Position& lhs, const Position& rhs) noexcept {
    return lhs.row == rhs.row && lhs.col == rhs.col;
}

bool operator!=(const Position& lhs, const Position& rhs) noexcept {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& out, const Position& pos) {
    return out << '(' << pos.row << ", " << pos.col << ')';
}

}  // namespace kfc
