#pragma once

#include <cstddef>
#include <cstdint>
#include <variant>

namespace kfc {

struct Select {
    std::size_t row;
    std::size_t col;
};

struct ClearSelection {};

struct MoveSelected {
    std::size_t to_row;
    std::size_t to_col;
};

struct JumpSelected {};

struct JumpAt {
    std::size_t row;
    std::size_t col;
};

struct AdvanceClock {
    std::int64_t milliseconds;
};

using GameAction = std::variant<Select, ClearSelection, MoveSelected, JumpSelected, JumpAt,
                                AdvanceClock>;

}  // namespace kfc
