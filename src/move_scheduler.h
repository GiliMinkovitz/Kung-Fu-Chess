#pragma once

#include "piece.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace kfc {

struct PendingMove {
    Piece piece;
    std::pair<std::size_t, std::size_t> start_pos;
    std::pair<std::size_t, std::size_t> end_pos;
    std::int64_t arrival_time = 0;
};

struct JumpState {
    Piece piece;
    std::pair<std::size_t, std::size_t> cell;
    std::int64_t arrival_time = 0;
};

class MoveScheduler {
public:
    [[nodiscard]] std::int64_t clock_ms() const noexcept { return clock_ms_; }
    [[nodiscard]] const std::vector<PendingMove>& pending_moves() const noexcept {
        return pending_moves_;
    }
    [[nodiscard]] const std::vector<JumpState>& active_jumps() const noexcept {
        return active_jumps_;
    }

    void add_clock(std::int64_t ms) noexcept { clock_ms_ += ms; }

    [[nodiscard]] bool is_piece_moving(std::size_t row, std::size_t col) const;
    [[nodiscard]] bool is_piece_jumping(std::size_t row, std::size_t col) const;
    [[nodiscard]] bool is_same_color_destination_claimed(
        char color, const std::pair<std::size_t, std::size_t>& end_pos) const;
    [[nodiscard]] bool conflicts_with_opposite_color_move(char moving_color,
                                                        const PendingMove& proposed) const;

    void schedule_move(PendingMove move);
    void schedule_jump(JumpState jump);
    void expire_jumps();
    void set_pending_moves(std::vector<PendingMove> moves);

private:
    std::vector<PendingMove> pending_moves_;
    std::vector<JumpState> active_jumps_;
    std::int64_t clock_ms_ = 0;
};

}  // namespace kfc
