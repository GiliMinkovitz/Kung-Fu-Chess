#include "model/board_model.h"
#include "model/game_config.h"
#include "server/match.h"

#include <chrono>
#include <iostream>
#include <thread>

namespace {

kfc::BoardModel default_board() {
    return kfc::BoardModel::from_token_grid({
        {"bR", "bN", "bB", "bQ", "bK", "bB", "bN", "bR"},
        {"bP", "bP", "bP", "bP", "bP", "bP", "bP", "bP"},
        {".", ".", ".", ".", ".", ".", ".", "."},
        {".", ".", ".", ".", ".", ".", ".", "."},
        {".", ".", ".", ".", ".", ".", ".", "."},
        {".", ".", ".", ".", ".", ".", ".", "."},
        {"wP", "wP", "wP", "wP", "wP", "wP", "wP", "wP"},
        {"wR", "wN", "wB", "wQ", "wK", "wB", "wN", "wR"},
    });
}

}  // namespace

int main() {
    kfc::Match match(default_board());

    auto last_tick = std::chrono::steady_clock::now();

    while (!match.is_game_over()) {
        const auto now = std::chrono::steady_clock::now();
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - last_tick).count();

        if (elapsed >= kfc::kTargetFrameMs) {
            match.tick(elapsed);

            std::cout << "Clock: " << match.state().clock_ms() << '\n';
            std::cout << "GameOver: " << (match.is_game_over() ? "true" : "false") << '\n';

            last_tick = now;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    return 0;
}
