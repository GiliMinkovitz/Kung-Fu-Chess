#include "ui_controller.h"

#include "board_view_builder.h"
#include "ui_window_config.h"

namespace kfc {

UiController::UiController(GameState& state, std::unique_ptr<IUiRenderer> renderer)
    : state_(state), processor_(state_), renderer_(std::move(renderer)) {
    renderer_->attach_input_sink(this);
    const UiWindowDimensions window = default_initial_window_size(state_.rows(), state_.cols());
    renderer_->init(window.width, window.height, state_.rows(), state_.cols());
    sync_input_layout();
}

UiFrameResult UiController::frame(std::int64_t delta_ms) {
    sync_input_layout();
    state_.add_clock(delta_ms);
    return renderer_->present(BoardViewBuilder::build(state_));
}

void UiController::sync_input_layout() {
    processor_.set_board_layout(renderer_->board_layout());
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
