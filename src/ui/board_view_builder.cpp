#include "board_view_builder.h"

#include "../realtime/render_snapshot.h"

namespace kfc {
BoardViewModel BoardViewBuilder::build(const GameState& state) {
    BoardViewModel view;
    view.rows = state.rows();
    view.cols = state.cols();
    view.clock_ms = state.clock_ms();
    view.game_over = state.is_game_over();

    if (state.has_selection()) {
        std::size_t row = 0;
        std::size_t col = 0;
        if (state.selection(row, col)) {
            view.selection = std::make_pair(row, col);
        }
    }

    view.cells.reserve(view.rows * view.cols);
    for (std::size_t row = 0; row < view.rows; ++row) {
        for (std::size_t col = 0; col < view.cols; ++col) {
            view.cells.push_back({row, col, state.token_at(row, col)});
        }
    }

    const AnimationSnapshot animations = state.animations_for_render();
    view.active_moves.reserve(animations.moves.size());
    for (const ActiveMoveSnapshot& move : animations.moves) {
        view.active_moves.push_back({
            move.piece_id,
            move.from_row,
            move.from_col,
            move.to_row,
            move.to_col,
            move.progress,
        });
    }

    view.active_jumps.reserve(animations.jumps.size());
    for (const ActiveJumpSnapshot& jump : animations.jumps) {
        view.active_jumps.push_back({
            jump.piece_id,
            jump.row,
            jump.col,
            jump.progress,
        });
    }

    return view;
}

}  // namespace kfc
