#include "board.h"

#include <iostream>

int main() {
    std::ios_base::sync_with_stdio(false);

    kfc::Board board;
    kfc::read_board(std::cin, board);

    if (kfc::is_valid_board(board)) {
        kfc::write_board(std::cout, board);
    }

    return 0;
}
