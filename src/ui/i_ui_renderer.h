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

[[nodiscard]] inline std::size_t board_view_cell_index(std::size_t row, std::size_t col,
                                                       std::size_t cols) noexcept {
    return row * cols + col;
}

[[nodiscard]] inline std::string board_view_token_at(const BoardViewModel& view, std::size_t row,
                                                     std::size_t col) {
    if (row >= view.rows || col >= view.cols) {
        return std::string(1, kEmptyToken);
    }

    const std::size_t index = board_view_cell_index(row, col, view.cols);
    if (index >= view.tokens.size()) {
        return std::string(1, kEmptyToken);
    }

    return view.tokens[index];
}

[[nodiscard]] inline bool board_view_is_move_origin(const BoardViewModel& view, std::size_t row,
                                                    std::size_t col) {
    for (const ActiveMoveSnapshot& move : view.animations.moves) {
        if (move.from_row == row && move.from_col == col) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] inline bool board_view_is_jumping_cell(const BoardViewModel& view, std::size_t row,
                                                     std::size_t col) {
    for (const ActiveJumpSnapshot& jump : view.animations.jumps) {
        if (jump.row == row && jump.col == col) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] inline float board_view_jump_progress_at(const BoardViewModel& view, std::size_t row,
                                                       std::size_t col) {
    for (const ActiveJumpSnapshot& jump : view.animations.jumps) {
        if (jump.row == row && jump.col == col) {
            return jump.progress;
        }
    }
    return 0.0f;
}

class IUiRenderer {
public:
    virtual ~IUiRenderer() = default;
    virtual void init(std::size_t rows, std::size_t cols, int cell_pixel_size) = 0;
    virtual void render(const BoardViewModel& view) = 0;
    virtual void shutdown() = 0;
};

}  // namespace kfc
