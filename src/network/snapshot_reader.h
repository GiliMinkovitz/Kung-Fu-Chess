#pragma once

#include "ui/view/board_view_model.h"

#include <optional>
#include <string_view>

namespace kfc {

// Parses the text snapshot format produced by write_snapshot() into a BoardViewModel.
// No networking, rendering, or game logic.
[[nodiscard]] std::optional<BoardViewModel> read_snapshot(std::string_view text);

#ifdef KFC_TEST_BUILD
namespace test {
[[nodiscard]] std::optional<std::int64_t> snapshot_parse_int64_for_tests(std::string_view token);
[[nodiscard]] std::optional<float> snapshot_parse_float_for_tests(std::string_view token);
}
#endif

}  // namespace kfc
