#include "ctd26_renderer.h"

#include "i_ui_input_sink.h"
#include "../model/game_config.h"

#include <img.hpp>

#include <algorithm>
#include <cmath>
#include <string>

namespace kfc {

struct Ctd26RendererImpl {
    std::unique_ptr<Img> frame;
    std::unique_ptr<Img> light_cell;
    std::unique_ptr<Img> dark_cell;
};

namespace {

constexpr const char* kWindowName = "Kung Fu Chess";
constexpr float kPi = 3.14159265f;

cv::Scalar to_scalar(const UiColor& color) {
    return cv::Scalar(color.b, color.g, color.r);
}

void on_mouse(int event, int x, int y, int /*flags*/, void* userdata) {
    auto* input_sink = static_cast<IUiInputSink*>(userdata);
    if (input_sink == nullptr) {
        return;
    }

    if (event == cv::EVENT_LBUTTONDOWN) {
        input_sink->on_pixel_click(x, y);
    } else if (event == cv::EVENT_RBUTTONDOWN) {
        input_sink->on_pixel_jump(x, y);
    }
}

}  // namespace

Ctd26Renderer::Ctd26Renderer(UiTheme theme) : theme_(theme), impl_(std::make_unique<Ctd26RendererImpl>()) {}

Ctd26Renderer::~Ctd26Renderer() = default;

void Ctd26Renderer::init(std::size_t rows, std::size_t cols, int cell_pixel_size) {
    rows_ = rows;
    cols_ = cols;
    cell_size_ = cell_pixel_size;

    const int width = static_cast<int>(cols_) * cell_size_;
    const int height = static_cast<int>(rows_) * cell_size_;

    impl_->frame = std::make_unique<Img>();
    impl_->light_cell = std::make_unique<Img>();
    impl_->dark_cell = std::make_unique<Img>();

    impl_->light_cell->create(cell_size_, cell_size_, to_scalar(theme_.light_cell));
    impl_->dark_cell->create(cell_size_, cell_size_, to_scalar(theme_.dark_cell));

    Img::open_window(kWindowName);
    Img::set_mouse_callback(kWindowName, on_mouse, input_sink_);

    impl_->frame->create(width, height, to_scalar(theme_.frame_background));
    impl_->frame->show_in(kWindowName);
    Img::poll_key(1);

    initialized_ = true;
}

void Ctd26Renderer::attach_input_sink(IUiInputSink* sink) {
    input_sink_ = sink;
    if (initialized_) {
        Img::set_mouse_callback(kWindowName, on_mouse, input_sink_);
    }
}

UiFrameResult Ctd26Renderer::present(const BoardViewModel& view) {
    if (!initialized_ || impl_ == nullptr || impl_->frame == nullptr || impl_->light_cell == nullptr ||
        impl_->dark_cell == nullptr) {
        return {true};
    }

    const int width = static_cast<int>(view.cols) * cell_size_;
    const int height = static_cast<int>(view.rows) * cell_size_;
    impl_->frame->create(width, height, to_scalar(theme_.frame_background));

    draw_static_board(view);
    draw_moving_tokens(view);
    draw_game_over_banner(view.game_over);

    impl_->frame->show_in(kWindowName);
    return {Img::poll_key(1) != 27};
}

void Ctd26Renderer::shutdown() {
    if (initialized_) {
        Img::close_window(kWindowName);
        initialized_ = false;
    }
}

void Ctd26Renderer::draw_cell_background(int x, int y, bool light) {
    if (light) {
        impl_->light_cell->draw_on(*impl_->frame, x, y);
    } else {
        impl_->dark_cell->draw_on(*impl_->frame, x, y);
    }
}

void Ctd26Renderer::draw_selection_highlight(int x, int y) {
    impl_->frame->rectangle(x, y, cell_size_, cell_size_, to_scalar(theme_.selection_border),
                            theme_.selection_border_thickness);
}

void Ctd26Renderer::draw_jump_effect(int x, int y, float jump_progress) {
    const int lift = static_cast<int>(std::sin(jump_progress * kPi) * cell_size_ * theme_.jump_lift_ratio);
    impl_->frame->rectangle(x, y - lift, cell_size_, cell_size_, to_scalar(theme_.jump_border),
                            theme_.jump_border_thickness);
}

void Ctd26Renderer::draw_token(const std::string& token, int x, int y) {
    if (token.empty() || token == std::string(1, kEmptyToken)) {
        return;
    }

    const cv::Scalar color =
        (!token.empty() && token.front() == kWhiteColor) ? to_scalar(theme_.white_token)
                                                         : to_scalar(theme_.black_token);
    const int text_x = x + cell_size_ / 6;
    const int text_y = y + (cell_size_ * 2) / 3;
    impl_->frame->put_text(token, text_x, text_y, theme_.token_font_scale, color, theme_.token_thickness);
}

void Ctd26Renderer::draw_static_board(const BoardViewModel& view) {
    for (std::size_t row = 0; row < view.rows; ++row) {
        for (std::size_t col = 0; col < view.cols; ++col) {
            const int x = static_cast<int>(col) * cell_size_;
            const int y = static_cast<int>(row) * cell_size_;
            const bool light = (row + col) % 2 == 0;

            draw_cell_background(x, y, light);

            if (view.selection && view.selection->first == row && view.selection->second == col) {
                draw_selection_highlight(x, y);
            }

            if (board_view_is_jumping_cell(view, row, col)) {
                draw_jump_effect(x, y, board_view_jump_progress_at(view, row, col));
            }

            if (board_view_is_move_origin(view, row, col)) {
                continue;
            }

            draw_token(board_view_token_at(view, row, col), x, y);
        }
    }
}

void Ctd26Renderer::draw_moving_tokens(const BoardViewModel& view) {
    for (const ActiveMoveSnapshot& move : view.animations.moves) {
        const std::string token = board_view_token_at(view, move.from_row, move.from_col);
        if (token.empty() || token == std::string(1, kEmptyToken)) {
            continue;
        }

        const float progress = std::clamp(move.progress, 0.0f, 1.0f);
        const float from_x = static_cast<float>(move.from_col * static_cast<std::size_t>(cell_size_));
        const float from_y = static_cast<float>(move.from_row * static_cast<std::size_t>(cell_size_));
        const float to_x = static_cast<float>(move.to_col * static_cast<std::size_t>(cell_size_));
        const float to_y = static_cast<float>(move.to_row * static_cast<std::size_t>(cell_size_));

        const int draw_x = static_cast<int>(from_x + (to_x - from_x) * progress);
        const int draw_y = static_cast<int>(from_y + (to_y - from_y) * progress);
        draw_token(token, draw_x, draw_y);
    }
}

void Ctd26Renderer::draw_game_over_banner(bool game_over) {
    if (!game_over) {
        return;
    }

    impl_->frame->put_text("GAME OVER", theme_.game_over_x, theme_.game_over_y, theme_.game_over_font_scale,
                           to_scalar(theme_.game_over_text), theme_.game_over_thickness);
}

}  // namespace kfc
