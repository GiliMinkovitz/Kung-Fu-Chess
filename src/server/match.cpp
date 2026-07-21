#include "match.h"

namespace kfc {

Match::Match(BoardModel board) : state_(std::move(board)) {}

Match::Match(BoardModel board, GameRules rules)
    : state_(std::move(board), std::move(rules)) {}

GameState& Match::state() noexcept {
    return state_;
}

const GameState& Match::state() const noexcept {
    return state_;
}

void Match::submit_action(const GameAction& action) {
    state_.apply_action(action);
}

void Match::tick(std::int64_t delta_ms) {
    state_.add_clock(delta_ms);
}

bool Match::is_game_over() const noexcept {
    return state_.is_game_over();
}

}  // namespace kfc
