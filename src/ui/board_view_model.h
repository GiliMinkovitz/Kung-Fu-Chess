#pragma once

#include "../model/game_config.h"
#include "../realtime/render_snapshot.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace kfc {

struct BoardViewModel {
    std::size_t rows = 0;
    std::size_t cols = 0;
    std::int64_t clock_ms = 0;
    bool game_over = false;
    std::optional<std::pair<std::size_t, std::size_t>> selection;
    std::vector<std::string> tokens;
    AnimationSnapshot animations;
};

[[nodiscard]] std::size_t board_view_cell_index(std::size_t row, std::size_t col,
                                                std::size_t cols) noexcept;

[[nodiscard]] std::string board_view_token_at(const BoardViewModel& view, std::size_t row,
                                              std::size_t col);

[[nodiscard]] bool board_view_is_move_origin(const BoardViewModel& view, std::size_t row,
                                             std::size_t col);

[[nodiscard]] bool board_view_is_jumping_cell(const BoardViewModel& view, std::size_t row,
                                              std::size_t col);

[[nodiscard]] float board_view_jump_progress_at(const BoardViewModel& view, std::size_t row,
                                                std::size_t col);

}  // namespace kfc
