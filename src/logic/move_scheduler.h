#pragma once

#include "../core/piece.h"

#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

namespace kfc {

struct PendingMove {
    Piece::Id piece_id = Piece::kInvalidId;
    PieceColor color = PieceColor::White;
    std::pair<std::size_t, std::size_t> start_pos;
    std::pair<std::size_t, std::size_t> end_pos;
    std::int64_t arrival_time = 0;
};

struct JumpState {
    Piece::Id piece_id = Piece::kInvalidId;
    PieceColor color = PieceColor::White;
    std::pair<std::size_t, std::size_t> cell;
    std::int64_t arrival_time = 0;
};

class BoardModel;
class CollisionResolver;
struct GameRules;
struct ArrivingPieceInfo;

class MoveScheduler {
public:
    void for_each_pending_due(uint64_t current_time_ms,
                              const std::function<void(const PendingMove&)>& fn);
    [[nodiscard]] bool check_for_jump_capture(
        uint64_t current_time_ms, const CollisionResolver& resolver, BoardModel& board,
        const GameRules& rules, const std::pair<std::size_t, std::size_t>& target_cell,
        const ArrivingPieceInfo& arriving_piece_info, bool& game_over) const;

    [[nodiscard]] bool is_piece_moving(uint64_t current_time_ms, std::size_t row,
                                       std::size_t col) const;
    [[nodiscard]] bool is_piece_jumping(uint64_t current_time_ms, std::size_t row,
                                        std::size_t col) const;
    [[nodiscard]] bool is_same_color_destination_claimed(
        uint64_t current_time_ms, PieceColor color,
        const std::pair<std::size_t, std::size_t>& end_pos) const;
    [[nodiscard]] bool conflicts_with_opposite_color_move(uint64_t current_time_ms,
                                                          PieceColor moving_color,
                                                          const PendingMove& proposed) const;

    void schedule_move(PendingMove move);
    void schedule_jump(JumpState jump);
    void expire_jumps(uint64_t current_time_ms);

private:
    std::vector<PendingMove> pending_moves_;
    std::vector<JumpState> active_jumps_;
};

}  // namespace kfc
