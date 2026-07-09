// Repository: https://github.com/GiliMinkovitz/Kung-Fu-Chess.git

#include "core/game_config.h"
#include "adapters/vpl_io.h"
#include "adapters/command_processor.h"
#include "logic/game_state.h"

#include <iostream>

int main() {
    std::ios_base::sync_with_stdio(false);

    const kfc::VplInput input = kfc::read_vpl_input(std::cin);

    if (input.error != kfc::BoardError::Ok) {
        std::cout << kfc::board_error_message(input.error) << '\n';
        return 0;
    }

    kfc::GameState state(input.board);
    kfc::CommandProcessor processor(state);

    for (const std::string& command : input.commands) {
        processor.execute(command, std::cout);
        if (command == kfc::kPrintBoardCommand) {
            std::cout << '\n';
        }
    }

    return 0;
}
