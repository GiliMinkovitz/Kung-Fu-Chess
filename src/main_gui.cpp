// Repository: https://github.com/GiliMinkovitz/Kung-Fu-Chess.git

#include "engine/game_engine.h"
#include "model/board_model.h"
#include "model/game_config.h"
#include "ui/ctd26_renderer.h"
#include "ui/ui_controller.h"

#include <chrono>
#include <iostream>
#include <memory>
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
    try {
        kfc::GameEngine engine(default_board());
        kfc::UiController controller(engine.state(), std::make_unique<kfc::Ctd26Renderer>());

        auto last_frame = std::chrono::steady_clock::now();

        while (true) {
            const auto now = std::chrono::steady_clock::now();
            const auto elapsed =
                std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame).count();
            if (elapsed >= kfc::kTargetFrameMs) {
                if (!controller.frame(elapsed).should_continue) {
                    break;
                }
                last_frame = now;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

        controller.shutdown();
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "GUI error: " << ex.what() << '\n';
        return 1;
    }
}
