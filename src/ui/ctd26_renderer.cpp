#include "ctd26_renderer.h"

namespace kfc {

void Ctd26Renderer::init(std::size_t rows, std::size_t cols, int cell_pixel_size) {
    rows_ = rows;
    cols_ = cols;
    cell_size_ = cell_pixel_size;
}

void Ctd26Renderer::render(const BoardViewModel& view) {
    (void)view;
    // Phase 3: draw view using CTD26.
}

void Ctd26Renderer::shutdown() {}

}  // namespace kfc
