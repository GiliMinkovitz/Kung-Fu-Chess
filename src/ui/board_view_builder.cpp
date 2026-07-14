#include "board_view_builder.h"

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

    return view;
}

}  // namespace kfc
