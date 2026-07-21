#pragma once

#include "logic/game_state.h"

namespace kfc {

class Match {
public:
    explicit Match(BoardModel board);
    Match(BoardModel board, GameRules rules);

    [[nodiscard]] GameState& state() noexcept;
    [[nodiscard]] const GameState& state() const noexcept;

    void submit_action(const GameAction& action);
    void tick(std::int64_t delta_ms);

    [[nodiscard]] bool is_game_over() const noexcept;

private:
    GameState state_;
};

}  // namespace kfc
