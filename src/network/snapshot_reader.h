#pragma once

#include "ui/view/board_view_model.h"

#include <optional>
#include <string_view>

namespace kfc {

// Parses the text snapshot format produced by write_snapshot() into a BoardViewModel.
// No networking, rendering, or game logic.
[[nodiscard]] std::optional<BoardViewModel> read_snapshot(std::string_view text);

}  // namespace kfc
