#include "ui_controller.h"

#include "board_view_builder.h"
#include "../model/game_config.h"

namespace kfc {

UiController::UiController(GameState& state, std::unique_ptr<IUiRenderer> renderer)
    : state_(state),
      processor_(state_, kCellPixelSize),
      renderer_(std::move(renderer)),
      cell_pixel_size_(kCellPixelSize) {
    renderer_->attach_input_sink(this);
    renderer_->init(state_.rows(), state_.cols(), cell_pixel_size_);
}

UiFrameResult UiController::frame(std::int64_t delta_ms) {
    state_.add_clock(delta_ms);
    return renderer_->present(BoardViewBuilder::build(state_));
}

void UiController::on_pixel_click(int x, int y) {
    processor_.handle_pixel_click(x, y);
}

void UiController::on_pixel_jump(int x, int y) {
    processor_.handle_pixel_jump(x, y);
}

void UiController::shutdown() {
    renderer_->shutdown();
}

}  // namespace kfc
