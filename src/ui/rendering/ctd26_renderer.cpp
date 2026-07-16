#include "ui/rendering/ctd26_renderer.h"

#include "ui/assets/asset_paths.h"
#include "ui/assets/image_loader.h"
#include "ui/rendering/i_ui_input_sink.h"
#include "model/piece_token.h"

#include <img.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <optional>
#include <string>

namespace kfc {

struct Ctd26RendererImpl {
    using PieceSpriteCache =
        std::array<std::array<std::optional<Img>, static_cast<std::size_t>(PieceKind::Count)>, 2>;

    std::unique_ptr<Img> frame;
    std::unique_ptr<Img> board_background;
    PieceSpriteCache piece_sprites{};
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

Ctd26Renderer::Ctd26Renderer(ThemeConfig theme_config, UiTheme ui_theme)
    : theme_config_(std::move(theme_config)), theme_(ui_theme), impl_(std::make_unique<Ctd26RendererImpl>()) {}

Ctd26Renderer::~Ctd26Renderer() = default;

void Ctd26Renderer::init(int window_width, int window_height, std::size_t rows, std::size_t cols) {
    window_width_ = window_width;
    window_height_ = window_height;
    board_width_cells_ = static_cast<int>(cols);
    board_height_cells_ = static_cast<int>(rows);

    recalculate_layout();

    impl_->frame = std::make_unique<Img>();
    impl_->board_background = std::make_unique<Img>();
    reload_board_background();
    reload_piece_sprites();

    Img::open_window(kWindowName);
    Img::set_mouse_callback(kWindowName, on_mouse, input_sink_);

    impl_->frame->create(window_width_, window_height_, to_scalar(theme_.frame_background));
    impl_->frame->show_in(kWindowName);
    Img::poll_key(1);

    initialized_ = true;
}

void Ctd26Renderer::recalculate_layout() {
    const int previous_cell_size = layout_.cell_size;
    layout_ = layout_calculator_.calculate(window_width_, window_height_, board_width_cells_,
                                           board_height_cells_);
    if (initialized_ && layout_.cell_size != previous_cell_size) {
        reload_piece_sprites();
    }
}

void Ctd26Renderer::reload_board_background() {
    const AssetPaths asset_paths(theme_config_);
    const ImageLoader image_loader(asset_paths);
    *impl_->board_background =
        image_loader.load_board({layout_.board_width_pixels, layout_.board_height_pixels});
}

void Ctd26Renderer::reload_piece_sprites() {
    impl_->piece_sprites = {};

    if (layout_.cell_size <= 0) {
        return;
    }

    const AssetPaths asset_paths(theme_config_);
    const ImageLoader image_loader(asset_paths);
    const std::pair<int, int> sprite_size{layout_.cell_size, layout_.cell_size};

    constexpr std::array<PieceColor, 2> kColors{PieceColor::White, PieceColor::Black};
    for (const PieceColor color : kColors) {
        const std::size_t color_index = color == PieceColor::White ? 0 : 1;
        for (std::size_t kind_index = 0; kind_index < static_cast<std::size_t>(PieceKind::Count);
             ++kind_index) {
            const auto kind = static_cast<PieceKind>(kind_index);
            try {
                impl_->piece_sprites[color_index][kind_index] =
                    image_loader.load_piece_sprite(color, kind, "idle", 1, sprite_size);
            } catch (const std::exception&) {
                impl_->piece_sprites[color_index][kind_index] = std::nullopt;
            }
        }
    }
}

void Ctd26Renderer::attach_input_sink(IUiInputSink* sink) {
    input_sink_ = sink;
    if (initialized_) {
        Img::set_mouse_callback(kWindowName, on_mouse, input_sink_);
    }
}

BoardLayout Ctd26Renderer::board_layout() const {
    return layout_;
}

UiFrameResult Ctd26Renderer::present(const BoardViewModel& view) {
    if (!initialized_ || impl_ == nullptr || impl_->frame == nullptr || impl_->board_background == nullptr) {
        return {true};
    }

    impl_->frame->create(window_width_, window_height_, to_scalar(theme_.frame_background));

    draw_static_board(view);
    draw_moving_pieces(view);
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

void Ctd26Renderer::draw_board_background() {
    impl_->board_background->draw_on(*impl_->frame, layout_.board_x, layout_.board_y);
}

void Ctd26Renderer::draw_selection_highlight(int x, int y) {
    impl_->frame->rectangle(x, y, layout_.cell_size, layout_.cell_size, to_scalar(theme_.selection_border),
                            layout_.selection_border_thickness());
}

void Ctd26Renderer::draw_jump_effect(int x, int y, float jump_progress) {
    const int lift = static_cast<int>(std::sin(jump_progress * kPi) *
                                      layout_.jump_lift_pixels(theme_.jump_lift_ratio));
    impl_->frame->rectangle(x, y - lift, layout_.cell_size, layout_.cell_size, to_scalar(theme_.jump_border),
                            layout_.jump_border_thickness());
}

void Ctd26Renderer::draw_piece(const PieceView& piece, int x, int y) {
    const std::size_t color_index = piece.color == PieceColor::White ? 0 : 1;
    const std::size_t kind_index = static_cast<std::size_t>(piece.kind);
    if (kind_index < static_cast<std::size_t>(PieceKind::Count)) {
        std::optional<Img>& sprite = impl_->piece_sprites[color_index][kind_index];
        if (sprite.has_value() && sprite->is_loaded()) {
            sprite->draw_on(*impl_->frame, x, y);
            return;
        }
    }

    const std::string label = std::string{color_to_char(piece.color), kind_to_char(piece.kind)};
    const cv::Scalar color =
        piece.color == PieceColor::White ? to_scalar(theme_.white_token) : to_scalar(theme_.black_token);
    impl_->frame->put_text(label, layout_.piece_text_x(x), layout_.piece_text_y(y), layout_.piece_font_scale(),
                           color, layout_.piece_font_thickness());
}

void Ctd26Renderer::draw_static_board(const BoardViewModel& view) {
    draw_board_background();

    for (std::size_t row = 0; row < view.height; ++row) {
        for (std::size_t col = 0; col < view.width; ++col) {
            const int x = layout_.cell_origin_x(col);
            const int y = layout_.cell_origin_y(row);

            if (view.selection && view.selection->first == row && view.selection->second == col) {
                draw_selection_highlight(x, y);
            }

            if (board_view_is_jumping_cell(view, row, col)) {
                draw_jump_effect(x, y, board_view_jump_progress_at(view, row, col));
            }

            if (board_view_is_move_origin(view, row, col)) {
                continue;
            }

            if (const std::optional<PieceView> piece = board_view_piece_at(view, row, col);
                piece.has_value()) {
                draw_piece(*piece, x, y);
            }
        }
    }
}

void Ctd26Renderer::draw_moving_pieces(const BoardViewModel& view) {
    for (const ActiveMoveSnapshot& move : view.animations.moves) {
        const std::optional<PieceView> piece = board_view_piece_at(view, move.from_row, move.from_col);
        if (!piece.has_value()) {
            continue;
        }

        const float progress = std::clamp(move.progress, 0.0f, 1.0f);
        const float from_x = static_cast<float>(layout_.cell_origin_x(move.from_col));
        const float from_y = static_cast<float>(layout_.cell_origin_y(move.from_row));
        const float to_x = static_cast<float>(layout_.cell_origin_x(move.to_col));
        const float to_y = static_cast<float>(layout_.cell_origin_y(move.to_row));

        const int draw_x = static_cast<int>(from_x + (to_x - from_x) * progress);
        const int draw_y = static_cast<int>(from_y + (to_y - from_y) * progress);
        draw_piece(*piece, draw_x, draw_y);
    }
}

void Ctd26Renderer::draw_game_over_banner(bool game_over) {
    if (!game_over) {
        return;
    }

    impl_->frame->put_text("GAME OVER", layout_.game_over_text_x(), layout_.game_over_text_y(),
                           layout_.game_over_font_scale(), to_scalar(theme_.game_over_text),
                           layout_.game_over_font_thickness());
}

}  // namespace kfc
