#pragma once

#include "board.h"

#include <cstdint>
#include <optional>
#include <utility>

namespace kfc {

class GameState {
public:
    explicit GameState(Board board);

    [[nodiscard]] const Board& board() const noexcept { return board_; }
    [[nodiscard]] std::int64_t clock_ms() const noexcept { return clock_ms_; }
    [[nodiscard]] bool has_selection() const noexcept { return selected_.has_value(); }

    [[nodiscard]] bool selection(std::size_t& row, std::size_t& col) const;
    [[nodiscard]] bool is_in_bounds(std::size_t row, std::size_t col) const noexcept;
    [[nodiscard]] bool is_piece(std::size_t row, std::size_t col) const;
    [[nodiscard]] bool is_friendly_to_selection(std::size_t row, std::size_t col) const;

    void add_clock(std::int64_t ms);
    void select(std::size_t row, std::size_t col);
    void clear_selection();
    void move_selected_to(std::size_t to_row, std::size_t to_col);

private:
    Board board_;
    std::optional<std::pair<std::size_t, std::size_t>> selected_;
    std::int64_t clock_ms_ = 0;
};

}  // namespace kfc
