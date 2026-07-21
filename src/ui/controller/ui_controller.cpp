#include "ui/controller/ui_controller.h"

#include "ui/layout/ui_window_config.h"
#include "ui/view/board_view_builder.h"

namespace kfc {

UiController::UiController(GameState& state, std::unique_ptr<IUiRenderer> renderer)
    : state_(&state), processor_(std::make_unique<GameInputHandler>(state)),
      renderer_(std::move(renderer)) {
    renderer_->attach_input_sink(this);
    const UiWindowDimensions window = default_initial_window_size(state.rows(), state.cols());
    renderer_->init(window.width, window.height, state.rows(), state.cols());
    sync_input_layout();
}

UiController::UiController(std::size_t rows, std::size_t cols,
                           std::unique_ptr<IUiRenderer> renderer)
    : renderer_(std::move(renderer)) {
    renderer_->attach_input_sink(this);
    const UiWindowDimensions window = default_initial_window_size(rows, cols);
    renderer_->init(window.width, window.height, rows, cols);
    sync_input_layout();
}

UiFrameResult UiController::frame(std::int64_t delta_ms) {
    sync_input_layout();
    state_->add_clock(delta_ms);
    return renderer_->present(BoardViewBuilder::build(*state_));
}

UiFrameResult UiController::present(const BoardViewModel& view) {
    sync_input_layout();
    return renderer_->present(view);
}

void UiController::sync_input_layout() {
    if (processor_ != nullptr) {
        processor_->set_board_layout(renderer_->board_layout());
    }
}

void UiController::on_pixel_click(int x, int y) {
    if (processor_ != nullptr) {
        processor_->handle_pixel_click(x, y);
    }
}

void UiController::on_pixel_jump(int x, int y) {
    if (processor_ != nullptr) {
        processor_->handle_pixel_jump(x, y);
    }
}

void UiController::shutdown() {
    renderer_->shutdown();
}

}  // namespace kfc
