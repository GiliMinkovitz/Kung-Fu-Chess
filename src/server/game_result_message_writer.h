#pragma once

#include <string>

namespace kfc {

enum class FinishReason {
    KingCapture,
    Resign,
    Disconnect,
};

[[nodiscard]] std::string create_game_result_message(bool won, FinishReason reason, int rating);

}  // namespace kfc
