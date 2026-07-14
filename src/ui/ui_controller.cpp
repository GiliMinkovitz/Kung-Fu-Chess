#include "ui_controller.h"

#include "board_view_builder.h"
#include "../model/game_config.h"

#include <sstream>
#include <string>

namespace kfc {

UiController::UiController(GameState& state, std::unique_ptr<IUiRenderer> renderer)
    : state_(state),
      processor_(state_),
      renderer_(std::move(renderer)),
      cell_pixel_size_(kCellPixelSize) {
    renderer_->init(state_.rows(), state_.cols(), cell_pixel_size_);
}

void UiController::tick(std::int64_t delta_ms) {
    state_.add_clock(delta_ms);
    renderer_->render(BoardViewBuilder::build(state_));
}

void UiController::on_pixel_click(int x, int y) {
    std::ostringstream sink;
    processor_.execute("click " + std::to_string(x) + " " + std::to_string(y), sink);
}

void UiController::on_pixel_jump(int x, int y) {
    std::ostringstream sink;
    processor_.execute("jump " + std::to_string(x) + " " + std::to_string(y), sink);
}

void UiController::shutdown() {
    renderer_->shutdown();
}

}  // namespace kfc
