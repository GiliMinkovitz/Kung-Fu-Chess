#pragma once

#include "board_layout.h"
#include "board_layout_calculator.h"
#include "i_ui_renderer.h"
#include "theme_config.h"
#include "ui_theme.h"

#include <memory>
namespace kfc {

struct Ctd26RendererImpl;

class Ctd26Renderer final : public IUiRenderer {
public:
    explicit Ctd26Renderer(ThemeConfig theme_config = kDefaultCtd26ThemeConfig,
                           UiTheme ui_theme = kDefaultUiTheme);
    ~Ctd26Renderer() override;

    Ctd26Renderer(const Ctd26Renderer&) = delete;
    Ctd26Renderer& operator=(const Ctd26Renderer&) = delete;

    void init(int window_width, int window_height, std::size_t rows, std::size_t cols) override;
    void attach_input_sink(IUiInputSink* sink) override;
    [[nodiscard]] BoardLayout board_layout() const override;
    UiFrameResult present(const BoardViewModel& view) override;
    void shutdown() override;

private:
    void recalculate_layout();
    void reload_board_background();

    void draw_board_background();
    void draw_selection_highlight(int x, int y);
    void draw_jump_effect(int x, int y, float jump_progress);
    void draw_piece(const PieceView& piece, int x, int y);
    void draw_static_board(const BoardViewModel& view);
    void draw_moving_pieces(const BoardViewModel& view);
    void draw_game_over_banner(bool game_over);

    ThemeConfig theme_config_;
    UiTheme theme_;
    BoardLayoutCalculator layout_calculator_;
    BoardLayout layout_{};
    int window_width_ = 0;
    int window_height_ = 0;
    int board_width_cells_ = 0;
    int board_height_cells_ = 0;
    bool initialized_ = false;
    IUiInputSink* input_sink_ = nullptr;
    std::unique_ptr<Ctd26RendererImpl> impl_;
};

}  // namespace kfc
