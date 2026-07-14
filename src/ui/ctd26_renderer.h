#pragma once

#include "i_ui_renderer.h"

#include <memory>
#include <string>

namespace kfc {

class UiController;

struct Ctd26RendererImpl;

class Ctd26Renderer final : public IUiRenderer {
public:
    Ctd26Renderer();
    ~Ctd26Renderer() override;

    Ctd26Renderer(const Ctd26Renderer&) = delete;
    Ctd26Renderer& operator=(const Ctd26Renderer&) = delete;

    void init(std::size_t rows, std::size_t cols, int cell_pixel_size) override;
    void render(const BoardViewModel& view) override;
    void shutdown() override;

    void attach_controller(UiController* controller);
    [[nodiscard]] bool poll_events();
    [[nodiscard]] UiController* controller_for_events() noexcept { return controller_; }

private:
    std::size_t rows_ = 0;
    std::size_t cols_ = 0;
    int cell_size_ = 0;
    bool initialized_ = false;
    UiController* controller_ = nullptr;
    std::unique_ptr<Ctd26RendererImpl> impl_;
};

}  // namespace kfc
