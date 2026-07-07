#pragma once

#include "board.h"

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace kfc {

struct PendingMove {
    std::string piece;
    std::pair<std::size_t, std::size_t> start_pos;
    std::pair<std::size_t, std::size_t> end_pos;
    std::int64_t arrival_time = 0;
};

struct JumpState {
    std::string piece;
    std::pair<std::size_t, std::size_t> cell;
    std::int64_t arrival_time = 0;
};

struct ArrivingPieceInfo {
    std::string piece;
    std::pair<std::size_t, std::size_t> start_pos;
    std::pair<std::size_t, std::size_t> end_pos;
};

class GameState {
public:
    static constexpr std::int64_t kMoveDurationMs = 1000;
    static constexpr std::int64_t kJumpDurationMs = 1000;

    explicit GameState(Board board);

    [[nodiscard]] const Board& board() const noexcept { return board_; }
    [[nodiscard]] std::int64_t clock_ms() const noexcept { return clock_ms_; }
    [[nodiscard]] bool has_selection() const noexcept { return selected_.has_value(); }
    [[nodiscard]] bool is_game_over() const noexcept { return game_over_; }

    [[nodiscard]] bool selection(std::size_t& row, std::size_t& col) const;
    [[nodiscard]] bool is_in_bounds(std::size_t row, std::size_t col) const noexcept;
    [[nodiscard]] bool is_piece(std::size_t row, std::size_t col) const;
    [[nodiscard]] bool is_piece_moving(std::size_t row, std::size_t col) const;
    [[nodiscard]] bool is_piece_jumping(std::size_t row, std::size_t col) const;
    [[nodiscard]] bool is_selectable_piece(std::size_t row, std::size_t col) const;
    [[nodiscard]] bool is_friendly_to_selection(std::size_t row, std::size_t col) const;
    [[nodiscard]] bool is_square_claimed_by_same_color_pending_move(std::size_t row,
                                                                    std::size_t col,
                                                                    char color) const;

    void add_clock(std::int64_t ms);
    void settle_pending_moves();
    void select(std::size_t row, std::size_t col);
    void clear_selection();
    void move_selected_to(std::size_t to_row, std::size_t to_col);
    void jump_selected();
    void jump_at(std::size_t row, std::size_t col);

private:
    void expire_jumps();
    [[nodiscard]] bool check_for_jump_capture(
        const std::pair<std::size_t, std::size_t>& target_cell,
        const ArrivingPieceInfo& arriving_piece_info);

    Board board_;
    std::optional<std::pair<std::size_t, std::size_t>> selected_;
    std::vector<PendingMove> pending_moves_;
    std::vector<JumpState> active_jumps_;
    std::int64_t clock_ms_ = 0;
    bool game_over_ = false;

};

}  // namespace kfc
