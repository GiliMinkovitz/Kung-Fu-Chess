#pragma once

#include "io/command_processor.h"
#include "logic/game_state.h"

#include <iosfwd>
#include <string>
#include <vector>

namespace kfc {

class GameEngine {
public:
    explicit GameEngine(BoardModel board);

    void execute(const std::string& command, std::ostream& out);
    void execute_all(const std::vector<std::string>& commands, std::ostream& out);

    [[nodiscard]] const GameState& state() const noexcept { return state_; }
    GameState& state() noexcept { return state_; }

private:
    GameState state_;
    CommandProcessor processor_;
};

}  // namespace kfc
