#include "game_engine.h"

#include "model/game_config.h"

#include <ostream>

namespace kfc {

GameEngine::GameEngine(BoardModel board) : state_(std::move(board)), processor_(state_) {}

void GameEngine::execute(const std::string& command, std::ostream& out) {
    processor_.execute(command, out);
    if (command == kPrintBoardCommand) {
        out << '\n';
    }
}

void GameEngine::execute_all(const std::vector<std::string>& commands, std::ostream& out) {
    for (const std::string& command : commands) {
        execute(command, out);
    }
}

}  // namespace kfc
