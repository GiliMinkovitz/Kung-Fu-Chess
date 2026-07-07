#include "board.h"

#include <iostream>

int main() {
    std::ios_base::sync_with_stdio(false);

    const kfc::VplInput input = kfc::read_vpl_input(std::cin);

    if (input.error != kfc::BoardError::Ok) {
        std::cout << kfc::board_error_message(input.error) << '\n';
        return 0;
    }

    for (const std::string& command : input.commands) {
        if (command == "print board") {
            kfc::write_board(std::cout, input.board);
            std::cout << '\n';
        }
    }

    return 0;
}
