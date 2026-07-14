#pragma once

#include "i_ui_renderer.h"

namespace kfc {

class Ctd26Renderer final : public IUiRenderer {
public:
    void init(std::size_t rows, std::size_t cols, int cell_pixel_size) override;
    void render(const BoardViewModel& view) override;
    void shutdown() override;

private:
    std::size_t rows_ = 0;
    std::size_t cols_ = 0;
    int cell_size_ = 0;
};

} // namespace kfc