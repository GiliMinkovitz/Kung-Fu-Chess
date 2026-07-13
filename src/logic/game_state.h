#pragma once

#include "../core/board_model.h"
#include "game_rules.h"
#include "real_time_arbiter.h"

#include <cstdint>
#include <functional>
#include <iosfwd>
#include <optional>
#include <string>
#include <utility>

namespace kfc {

namespace test {
struct GameStateTestAccess;
}

class GameState {
    friend struct test::GameStateTestAccess;
public:
    explicit GameState(BoardModel board);
    GameState(BoardModel board, GameRules rules);

    [[nodiscard]] const GameRules& rules() const noexcept { return rules_; }
    [[nodiscard]] std::size_t rows() const noexcept { return board_.rows(); }
    [[nodiscard]] std::size_t cols() const noexcept { return board_.cols(); }
    [[nodiscard]] std::string token_at(std::size_t row, std::size_t col) const;

    [[nodiscard]] std::int64_t clock_ms() const noexcept { return arbiter_.clock_ms(); }
    [[nodiscard]] bool has_selection() const noexcept { return selected_.has_value(); }
    [[nodiscard]] bool is_game_over() const noexcept { return game_over_; }
    [[nodiscard]] bool same_board_layout_as(const GameState& other) const noexcept;

    [[nodiscard]] bool selection(std::size_t& row, std::size_t& col) const;
    [[nodiscard]] bool is_in_bounds(std::size_t row, std::size_t col) const noexcept;
    [[nodiscard]] bool is_piece(std::size_t row, std::size_t col) const;
    [[nodiscard]] bool is_piece_moving(std::size_t row, std::size_t col) const;
    [[nodiscard]] bool is_piece_jumping(std::size_t row, std::size_t col) const;
    [[nodiscard]] bool is_selectable_piece(std::size_t row, std::size_t col) const;
    [[nodiscard]] bool is_friendly_to_selection(std::size_t row, std::size_t col) const;

    void add_clock(std::int64_t ms);
    void settle_pending_moves();
    void select(std::size_t row, std::size_t col);
    void clear_selection();
    void move_selected_to(std::size_t to_row, std::size_t to_col);
    void jump_selected();
    void jump_at(std::size_t row, std::size_t col);
    void write_board(std::ostream& out,
                     const std::function<void(std::ostream&, const BoardModel&)>& writer);

private:
    [[nodiscard]] bool is_legal_move(int start_row, int start_col, int end_row,
                                     int end_col) const;
    [[nodiscard]] bool can_move_selected_to(std::size_t from_row, std::size_t from_col,
                                            std::size_t to_row, std::size_t to_col) const;
    [[nodiscard]] std::int64_t compute_move_duration(std::size_t from_row, std::size_t from_col,
                                                     std::size_t to_row, std::size_t to_col,
                                                     bool is_capture) const noexcept;

    BoardModel board_;
    GameRules rules_;
    RealTimeArbiter arbiter_;
    std::optional<std::pair<std::size_t, std::size_t>> selected_;
    bool game_over_ = false;
};

}  // namespace kfc
