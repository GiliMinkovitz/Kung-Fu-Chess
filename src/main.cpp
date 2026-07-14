// Repository: https://github.com/GiliMinkovitz/Kung-Fu-Chess.git

#include "engine/game_engine.h"
#include "io/vpl_io.h"
#include "model/game_config.h"

#include <iostream>

int main() {
    std::ios_base::sync_with_stdio(false);

    const kfc::VplInput input = kfc::read_vpl_input(std::cin);

    if (input.error != kfc::BoardError::Ok) {
        std::cout << kfc::board_error_message(input.error) << '\n';
        return 0;
    }

    kfc::GameEngine engine(input.board);
    engine.execute_all(input.commands, std::cout);

    return 0;
}
