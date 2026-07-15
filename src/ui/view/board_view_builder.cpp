#include "ui/view/board_view_builder.h"

#include "model/piece_token.h"

namespace kfc {

BoardViewModel BoardViewBuilder::build(const GameState& state) {
    BoardViewModel view;
    view.height = state.rows();
    view.width = state.cols();
    view.clock_ms = state.clock_ms();
    view.game_over = state.is_game_over();

    if (state.has_selection()) {
        std::size_t row = 0;
        std::size_t col = 0;
        if (state.selection(row, col)) {
            view.selection = std::make_pair(row, col);
        }
    }

    view.cells.reserve(view.height * view.width);
    for (std::size_t row = 0; row < view.height; ++row) {
        for (std::size_t col = 0; col < view.width; ++col) {
            CellView cell;
            const std::optional<PieceDescriptor> descriptor =
                descriptor_from_token(state.token_at(row, col));
            if (descriptor.has_value()) {
                cell.piece = PieceView{descriptor->kind, descriptor->color};
            }
            view.cells.push_back(cell);
        }
    }

    view.animations = state.animations_for_render();
    return view;
}

}  // namespace kfc
