#pragma once

#include "network/game_redirect_info.h"

#include <optional>
#include <string_view>

namespace kfc {

[[nodiscard]] std::optional<GameRedirectInfo> read_game_redirect_message(std::string_view text);

}  // namespace kfc
