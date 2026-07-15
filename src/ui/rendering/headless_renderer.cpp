#include "ui/rendering/headless_renderer.h"

namespace kfc {

void HeadlessRenderer::init(int window_width, int window_height, std::size_t rows, std::size_t cols) {
    rows_ = rows;
    cols_ = cols;
    window_width_ = window_width;
    window_height_ = window_height;
    layout_ = layout_calculator_.calculate(window_width_, window_height_, static_cast<int>(cols),
                                           static_cast<int>(rows));
    initialized_ = true;
}

void HeadlessRenderer::attach_input_sink(IUiInputSink* sink) {
    input_sink_ = sink;
}

BoardLayout HeadlessRenderer::board_layout() const {
    return layout_;
}

UiFrameResult HeadlessRenderer::present(const BoardViewModel& view) {
    last_view_ = view;
    ++present_count_;
    return {should_continue_};
}

void HeadlessRenderer::shutdown() {
    shutdown_called_ = true;
    initialized_ = false;
}

}  // namespace kfc
