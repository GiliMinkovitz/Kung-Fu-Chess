#pragma once

#include "i_ui_renderer.h"
#include "ui_theme.h"

#include <memory>
#include <string>

namespace kfc {

struct Ctd26RendererImpl;

class Ctd26Renderer final : public IUiRenderer {
public:
    explicit Ctd26Renderer(UiTheme theme = kDefaultUiTheme);
    ~Ctd26Renderer() override;

    Ctd26Renderer(const Ctd26Renderer&) = delete;
    Ctd26Renderer& operator=(const Ctd26Renderer&) = delete;

    void init(std::size_t rows, std::size_t cols, int cell_pixel_size) override;
    void attach_input_sink(IUiInputSink* sink) override;
    UiFrameResult present(const BoardViewModel& view) override;
    void shutdown() override;

private:
    void draw_cell_background(int x, int y, bool light);
    void draw_selection_highlight(int x, int y);
    void draw_jump_effect(int x, int y, float jump_progress);
    void draw_token(const std::string& token, int x, int y);
    void draw_static_board(const BoardViewModel& view);
    void draw_moving_tokens(const BoardViewModel& view);
    void draw_game_over_banner(bool game_over);

    UiTheme theme_;
    std::size_t rows_ = 0;
    std::size_t cols_ = 0;
    int cell_size_ = 0;
    bool initialized_ = false;
    IUiInputSink* input_sink_ = nullptr;
    std::unique_ptr<Ctd26RendererImpl> impl_;
};

}  // namespace kfc
