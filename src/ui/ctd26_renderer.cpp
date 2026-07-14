#include "ctd26_renderer.h"

#include "ui_controller.h"
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

cv::Scalar token_color(const std::string& token) {
    if (!token.empty() && token.front() == kWhiteColor) {
        return cv::Scalar(20, 20, 20);
    }
    return cv::Scalar(240, 240, 240);
}

void on_mouse(int event, int x, int y, int /*flags*/, void* userdata) {
    auto* renderer = static_cast<Ctd26Renderer*>(userdata);
    if (renderer == nullptr) {
        return;
    }

    UiController* controller = renderer->controller_for_events();
    if (controller == nullptr) {
        return;
    }

    if (event == cv::EVENT_LBUTTONDOWN) {
        controller->on_pixel_click(x, y);
    } else if (event == cv::EVENT_RBUTTONDOWN) {
        controller->on_pixel_jump(x, y);
    }
}

}  // namespace

Ctd26Renderer::Ctd26Renderer() : impl_(std::make_unique<Ctd26RendererImpl>()) {}

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

    impl_->light_cell->create(cell_size_, cell_size_, cv::Scalar(220, 220, 220));
    impl_->dark_cell->create(cell_size_, cell_size_, cv::Scalar(100, 100, 100));

    Img::open_window(kWindowName);
    Img::set_mouse_callback(kWindowName, on_mouse, this);

    impl_->frame->create(width, height, cv::Scalar(30, 30, 30));
    impl_->frame->show_in(kWindowName);
    Img::poll_key(1);

    initialized_ = true;
}

void Ctd26Renderer::render(const BoardViewModel& view) {
    if (!initialized_ || impl_ == nullptr || impl_->frame == nullptr || impl_->light_cell == nullptr ||
        impl_->dark_cell == nullptr) {
        return;
    }

    const int width = static_cast<int>(view.cols) * cell_size_;
    const int height = static_cast<int>(view.rows) * cell_size_;
    impl_->frame->create(width, height, cv::Scalar(30, 30, 30));

    for (std::size_t row = 0; row < view.rows; ++row) {
        for (std::size_t col = 0; col < view.cols; ++col) {
            const int x = static_cast<int>(col) * cell_size_;
            const int y = static_cast<int>(row) * cell_size_;
            const bool light = (row + col) % 2 == 0;

            if (light) {
                impl_->light_cell->draw_on(*impl_->frame, x, y);
            } else {
                impl_->dark_cell->draw_on(*impl_->frame, x, y);
            }

            if (view.selection && view.selection->first == row && view.selection->second == col) {
                impl_->frame->rectangle(x, y, cell_size_, cell_size_, cv::Scalar(0, 220, 255), 3);
            }

            if (board_view_is_jumping_cell(view, row, col)) {
                const float jump_progress = board_view_jump_progress_at(view, row, col);
                const int lift =
                    static_cast<int>(std::sin(jump_progress * 3.14159265f) * cell_size_ * 0.25f);
                impl_->frame->rectangle(x, y - lift, cell_size_, cell_size_, cv::Scalar(0, 180, 255), 2);
            }

            if (board_view_is_move_origin(view, row, col)) {
                continue;
            }

            const std::string token = board_view_token_at(view, row, col);
            if (token.empty() || token == std::string(1, kEmptyToken)) {
                continue;
            }

            const int text_x = x + cell_size_ / 6;
            const int text_y = y + (cell_size_ * 2) / 3;
            impl_->frame->put_text(token, text_x, text_y, 1.2, token_color(token), 2);
        }
    }

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

        const int draw_x =
            static_cast<int>(from_x + (to_x - from_x) * progress) + cell_size_ / 6;
        const int draw_y =
            static_cast<int>(from_y + (to_y - from_y) * progress) + (cell_size_ * 2) / 3;

        impl_->frame->put_text(token, draw_x, draw_y, 1.2, token_color(token), 2);
    }

    if (view.game_over) {
        impl_->frame->put_text("GAME OVER", 20, 40, 1.0, cv::Scalar(0, 0, 255), 2);
    }

    impl_->frame->show_in(kWindowName);
    Img::poll_key(1);
}

void Ctd26Renderer::shutdown() {
    if (initialized_) {
        Img::close_window(kWindowName);
        initialized_ = false;
    }
}

void Ctd26Renderer::attach_controller(UiController* controller) {
    controller_ = controller;
}

bool Ctd26Renderer::poll_events() {
    return Img::poll_key(1) != 27;
}

}  // namespace kfc
