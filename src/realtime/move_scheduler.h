#pragma once

#include "../model/piece.h"

#include "render_snapshot.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace kfc {

// A move in flight: the piece remains at start_pos on the board until settlement.
// Invariant: start_time <= arrival_time; only moves with clock < arrival_time are active.
struct PendingMove {
    Piece::Id piece_id = Piece::kInvalidId;
    PieceColor color = PieceColor::White;
    std::pair<std::size_t, std::size_t> start_pos;
    std::pair<std::size_t, std::size_t> end_pos;
    std::int64_t start_time = 0;
    std::int64_t arrival_time = 0;
};

// Jump invulnerability window at a fixed cell; checked at move arrival, not on request.
struct JumpState {
    Piece::Id piece_id = Piece::kInvalidId;
    PieceColor color = PieceColor::White;
    std::pair<std::size_t, std::size_t> cell;
    std::int64_t start_time = 0;
    std::int64_t arrival_time = 0;
};

struct ActiveRest {
    RestKind kind;
    int start_time_ms;
    int end_time_ms;
    std::size_t row = 0;
    std::size_t col = 0;
};

class BoardModel;
class CollisionResolver;
struct GameRules;
struct ArrivingPieceInfo;

// Time-indexed queues for in-flight moves and jumps. Stores schedules and answers
// temporal queries; does not interpret chess rules except by forwarding to
// CollisionResolver. Board mutation happens only in RealTimeArbiter's settle callback.
class MoveScheduler {
public:
    // Invokes fn for each due move and removes them from pending_moves_.
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
    void schedule_rest(Piece::Id piece_id, RestKind kind, int start_time_ms, int end_time_ms,
                       std::size_t row, std::size_t col);
    void clear_rest(Piece::Id piece_id);
    void expire_jumps(
        uint64_t current_time_ms,
        const std::function<void(const JumpState&)>& on_complete = {});
    void expire_rests(uint64_t current_time_ms);

    [[nodiscard]] bool is_piece_resting(uint64_t current_time_ms, Piece::Id piece_id) const;
    [[nodiscard]] std::optional<RestKind> rest_kind(uint64_t current_time_ms,
                                                      Piece::Id piece_id) const;
    [[nodiscard]] AnimationSnapshot animations_at(std::int64_t clock_ms) const;

private:
    std::vector<PendingMove> pending_moves_;
    std::vector<JumpState> active_jumps_;
    std::unordered_map<Piece::Id, ActiveRest> active_rests_;
};

}  // namespace kfc
