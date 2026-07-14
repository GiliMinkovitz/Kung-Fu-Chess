#include "headless_renderer.h"

namespace kfc {

void HeadlessRenderer::init(std::size_t rows, std::size_t cols, int cell_pixel_size) {
    rows_ = rows;
    cols_ = cols;
    cell_pixel_size_ = cell_pixel_size;
    initialized_ = true;
    shutdown_called_ = false;
}

void HeadlessRenderer::attach_input_sink(IUiInputSink* sink) {
    input_sink_ = sink;
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
