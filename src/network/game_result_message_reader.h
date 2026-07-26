#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace kfc {

struct GameResult {
    bool won = false;
    std::string reason;
    int rating = 0;
};

std::optional<GameResult> read_game_result_message(std::string_view text);

}  // namespace kfc
