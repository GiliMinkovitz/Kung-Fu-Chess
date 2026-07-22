#pragma once

#include <optional>
#include <string_view>

namespace kfc {

enum class MatchmakingState {
    Idle,
    Searching,
    MatchedWhite,
    MatchedBlack,
    Timeout,
};

// Parses server matchmaking status messages. Returns nullopt when text is not a
// recognized matchmaking message.
[[nodiscard]] std::optional<MatchmakingState> read_matchmaking_message(std::string_view text);

}  // namespace kfc
