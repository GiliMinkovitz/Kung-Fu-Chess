#include "ui/rendering/ctd26_renderer.h"

#include "ui/assets/asset_paths.h"
#include "ui/assets/image_loader.h"
#include "ui/assets/sprite_animation_constants.h"
#include "ui/rendering/i_ui_input_sink.h"
#include "model/piece_token.h"

#include <img.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>

namespace kfc {

struct PieceSpriteCacheKey {
    PieceColor color;
    PieceKind kind;
    std::string state;
    int frame;

    bool operator==(const PieceSpriteCacheKey& other) const {
        return color == other.color && kind == other.kind && state == other.state &&
               frame == other.frame;
    }
};

struct PieceSpriteCacheKeyHash {
    std::size_t operator()(const PieceSpriteCacheKey& key) const {
        const std::size_t color_hash = std::hash<int>{}(static_cast<int>(key.color));
        const std::size_t kind_hash = std::hash<int>{}(static_cast<int>(key.kind));
        const std::size_t state_hash = std::hash<std::string>{}(key.state);
        const std::size_t frame_hash = std::hash<int>{}(key.frame);
        return color_hash ^ (kind_hash << 1) ^ (state_hash << 2) ^ (frame_hash << 3);
    }
};

struct Ctd26RendererImpl {
    using PieceSpriteCache =
        std::unordered_map<PieceSpriteCacheKey, std::optional<Img>, PieceSpriteCacheKeyHash>;

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

const char* diag_rest_kind_name(RestKind rest_kind) {
    return rest_kind == RestKind::Long ? "Long" : "Short";
}

void log_render_diag_exception(const char* function_name, const std::exception& ex,
                               std::optional<Piece::Id> piece_id = std::nullopt,
                               std::optional<std::size_t> row = std::nullopt,
                               std::optional<std::size_t> col = std::nullopt,
                               std::optional<RestKind> rest_kind = std::nullopt,
                               std::optional<float> progress = std::nullopt) {
    std::cerr << "[REST-RENDER-DIAG] EXCEPTION"
              << " function=" << function_name << " type=" << typeid(ex).name()
              << " what=" << ex.what();
    if (piece_id.has_value()) {
        std::cerr << " piece_id=" << *piece_id;
    }
    if (row.has_value()) {
        std::cerr << " row=" << *row;
    }
    if (col.has_value()) {
        std::cerr << " col=" << *col;
    }
    if (rest_kind.has_value()) {
        std::cerr << " rest_kind=" << diag_rest_kind_name(*rest_kind);
    }
    if (progress.has_value()) {
        std::cerr << " progress=" << *progress;
    }
    std::cerr << '\n';
}

std::unordered_set<Piece::Id>& logged_rest_piece_ids() {
    static std::unordered_set<Piece::Id> ids;
    return ids;
}

bool should_log_rest_diagnostics(Piece::Id piece_id) {
    return logged_rest_piece_ids().insert(piece_id).second;
}

void clear_rest_diagnostic_state() {
    logged_rest_piece_ids().clear();
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

    cv::namedWindow(kWindowName, cv::WINDOW_NORMAL);
    cv::resizeWindow(kWindowName, window_width_, window_height_);
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
    impl_->piece_sprites.clear();

    if (layout_.cell_size <= 0) {
        return;
    }

    const AssetPaths asset_paths(theme_config_);
    const ImageLoader image_loader(asset_paths);
    const std::pair<int, int> sprite_size{layout_.cell_size, layout_.cell_size};

    const auto load_and_cache = [&](PieceColor color, PieceKind kind, const std::string& state,
                                    int frame) {
        const PieceSpriteCacheKey key{color, kind, state, frame};
        try {
            impl_->piece_sprites[key] =
                image_loader.load_piece_sprite(color, kind, state, frame, sprite_size);
        } catch (const std::exception&) {
            impl_->piece_sprites[key] = std::nullopt;
        }
    };

    constexpr std::array<PieceColor, 2> kColors{PieceColor::White, PieceColor::Black};
    for (const PieceColor color : kColors) {
        for (std::size_t kind_index = 0; kind_index < static_cast<std::size_t>(PieceKind::Count);
             ++kind_index) {
            const auto kind = static_cast<PieceKind>(kind_index);

            load_and_cache(color, kind, std::string(kSpriteStateIdle), kSpriteIdleFrame);

            for (int frame = 1; frame <= kSpriteAnimationFrameCount; ++frame) {
                load_and_cache(color, kind, std::string(kSpriteStateMove), frame);
                load_and_cache(color, kind, std::string(kSpriteStateJump), frame);
                load_and_cache(color, kind, std::string(kSpriteStateShortRest), frame);
                load_and_cache(color, kind, std::string(kSpriteStateLongRest), frame);
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

    try {
        draw_static_board(view);
    } catch (const std::exception& ex) {
        log_render_diag_exception("Ctd26Renderer::present->draw_static_board", ex);
        throw;
    }
    try {
        draw_moving_pieces(view);
    } catch (const std::exception& ex) {
        log_render_diag_exception("Ctd26Renderer::present->draw_moving_pieces", ex);
        throw;
    }
    try {
        draw_jumping_pieces(view);
    } catch (const std::exception& ex) {
        log_render_diag_exception("Ctd26Renderer::present->draw_jumping_pieces", ex);
        throw;
    }
    try {
        draw_game_over_banner(view.game_over);
    } catch (const std::exception& ex) {
        log_render_diag_exception("Ctd26Renderer::present->draw_game_over_banner", ex);
        throw;
    }

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

void Ctd26Renderer::draw_rest_cooldown_overlay(int x, int y, float progress, RestKind rest_kind) {
    try {
        const float remaining = 1.0f - std::clamp(progress, 0.0f, 1.0f);
        if (remaining <= 0.0f) {
            return;
        }

        const int inset = std::max(1, layout_.cell_size / 16);
        const int inner_size = layout_.cell_size - 2 * inset;
        if (inner_size <= 0) {
            return;
        }

        const int overlay_height =
            std::max(1, static_cast<int>(std::ceil(static_cast<float>(inner_size) * remaining)));
        const int overlay_x = x + inset;
        const int overlay_y = y + layout_.cell_size - inset - overlay_height;

        const int intensity = static_cast<int>(40.0f + 70.0f * remaining);
        const cv::Scalar color =
            rest_kind == RestKind::Long ? cv::Scalar(intensity + 15, intensity - 10, intensity - 30)
                                        : cv::Scalar(intensity + 25, intensity + 8, intensity - 15);

        impl_->frame->rectangle(overlay_x, overlay_y, inner_size, overlay_height, color, -1);
    } catch (const std::exception& ex) {
        log_render_diag_exception("Ctd26Renderer::draw_rest_cooldown_overlay", ex, std::nullopt,
                                  std::nullopt, std::nullopt, rest_kind, progress);
        throw;
    }
}

void Ctd26Renderer::draw_piece(const PieceSpriteContext& context, int x, int y) {
    try {
        const PieceSpriteSelection selection = sprite_selector_.select(context);
        const PieceView& piece = context.piece;
        const PieceSpriteCacheKey key{piece.color, piece.kind, std::string(selection.state),
                                      selection.frame};
        const auto cached_sprite = impl_->piece_sprites.find(key);
        if (cached_sprite != impl_->piece_sprites.end() && cached_sprite->second.has_value() &&
            cached_sprite->second->is_loaded()) {
            cached_sprite->second->draw_on(*impl_->frame, x, y);
            return;
        }

        const std::string label =
            std::string{color_to_char(piece.color), kind_to_char(piece.kind)};
        const cv::Scalar color = piece.color == PieceColor::White ? to_scalar(theme_.white_token)
                                                                   : to_scalar(theme_.black_token);
        impl_->frame->put_text(label, layout_.piece_text_x(x), layout_.piece_text_y(y),
                               layout_.piece_font_scale(), color, layout_.piece_font_thickness());
    } catch (const std::exception& ex) {
        log_render_diag_exception("Ctd26Renderer::draw_piece", ex, std::nullopt, std::nullopt,
                                  std::nullopt, context.rest_kind, context.progress);
        throw;
    }
}

void Ctd26Renderer::draw_static_board(const BoardViewModel& view) {
    const bool has_active_rests = !view.animations.rests.empty();
    if (!has_active_rests) {
        clear_rest_diagnostic_state();
    } else if (logged_rest_piece_ids().empty()) {
        std::cerr << "[REST-RENDER-DIAG] entering rest rendering path, active_rests="
                  << view.animations.rests.size() << '\n';
    }
    try {
        draw_board_background();

        for (std::size_t row = 0; row < view.height; ++row) {
            for (std::size_t col = 0; col < view.width; ++col) {
                const int x = layout_.cell_origin_x(col);
                const int y = layout_.cell_origin_y(row);

                if (view.selection && view.selection->first == row &&
                    view.selection->second == col) {
                    draw_selection_highlight(x, y);
                }

                if (board_view_is_resting_cell(view, row, col)) {
                    const float progress = board_view_rest_progress_at(view, row, col);
                    const RestKind rest_kind = board_view_rest_kind_at(view, row, col);
                    std::optional<Piece::Id> piece_id;
                    for (const ActiveRestSnapshot& rest : view.animations.rests) {
                        if (rest.row == row && rest.col == col) {
                            piece_id = rest.piece_id;
                            break;
                        }
                    }
                    try {
                        draw_rest_cooldown_overlay(x, y, progress, rest_kind);
                    } catch (const std::exception& ex) {
                        log_render_diag_exception("Ctd26Renderer::draw_static_board->draw_rest_cooldown_overlay",
                                                  ex, piece_id, row, col, rest_kind, progress);
                        throw;
                    }
                }

                if (board_view_is_move_origin(view, row, col) ||
                    board_view_is_jump_origin(view, row, col)) {
                    continue;
                }

                if (const std::optional<PieceView> piece = board_view_piece_at(view, row, col);
                    piece.has_value()) {
                    if (board_view_is_resting_cell(view, row, col)) {
                        const float progress = board_view_rest_progress_at(view, row, col);
                        const RestKind rest_kind = board_view_rest_kind_at(view, row, col);
                        std::optional<Piece::Id> piece_id;
                        for (const ActiveRestSnapshot& rest : view.animations.rests) {
                            if (rest.row == row && rest.col == col) {
                                piece_id = rest.piece_id;
                                break;
                            }
                        }
                        if (has_active_rests && piece_id.has_value() &&
                            should_log_rest_diagnostics(*piece_id)) {
                            const PieceSpriteContext rest_context{*piece, false, false, true,
                                                                  rest_kind, progress};
                            const PieceSpriteSelection selection =
                                sprite_selector_.select(rest_context);
                            const PieceSpriteCacheKey key{
                                piece->color, piece->kind, std::string(selection.state),
                                selection.frame};
                            const auto cached_sprite = impl_->piece_sprites.find(key);
                            const bool cache_hit =
                                cached_sprite != impl_->piece_sprites.end() &&
                                cached_sprite->second.has_value() &&
                                cached_sprite->second->is_loaded();
                            std::cerr << "[REST-RENDER-DIAG] resting cell piece_id=" << *piece_id
                                      << " row=" << row << " col=" << col
                                      << " rest_kind=" << diag_rest_kind_name(rest_kind)
                                      << " progress=" << progress << " state=" << selection.state
                                      << " frame=" << selection.frame
                                      << " cache=" << (cache_hit ? "hit" : "miss");
                            if (cache_hit) {
                                const cv::Mat& sprite_mat = cached_sprite->second->get_mat();
                                std::cerr << " sprite=" << sprite_mat.cols << "x"
                                          << sprite_mat.rows;
                            }
                            std::cerr << " dest_x=" << x << " dest_y=" << y << '\n';
                        }
                        try {
                            draw_piece(
                                PieceSpriteContext{*piece, false, false, true, rest_kind, progress},
                                x, y);
                        } catch (const std::exception& ex) {
                            log_render_diag_exception("Ctd26Renderer::draw_static_board->draw_piece",
                                                      ex, piece_id, row, col, rest_kind, progress);
                            throw;
                        }
                    } else {
                        draw_piece(PieceSpriteContext{*piece}, x, y);
                    }
                }
            }
        }
    } catch (const std::exception& ex) {
        log_render_diag_exception("Ctd26Renderer::draw_static_board", ex);
        throw;
    }
}

void Ctd26Renderer::draw_jumping_pieces(const BoardViewModel& view) {
    for (const ActiveJumpSnapshot& jump : view.animations.jumps) {
        const float progress = std::clamp(jump.progress, 0.0f, 1.0f);
        const int x = layout_.cell_origin_x(jump.col);
        const int y = layout_.cell_origin_y(jump.row);
        draw_jump_effect(x, y, progress);
        const PieceView piece{jump.kind, jump.color};
        draw_piece(PieceSpriteContext{piece, false, true, false, RestKind::Short, progress}, x, y);
    }
}

void Ctd26Renderer::draw_moving_pieces(const BoardViewModel& view) {
    for (const ActiveMoveSnapshot& move : view.animations.moves) {
        const float progress = std::clamp(move.progress, 0.0f, 1.0f);
        const float from_x = static_cast<float>(layout_.cell_origin_x(move.from_col));
        const float from_y = static_cast<float>(layout_.cell_origin_y(move.from_row));
        const float to_x = static_cast<float>(layout_.cell_origin_x(move.to_col));
        const float to_y = static_cast<float>(layout_.cell_origin_y(move.to_row));

        const int draw_x = static_cast<int>(from_x + (to_x - from_x) * progress);
        const int draw_y = static_cast<int>(from_y + (to_y - from_y) * progress);
        const PieceView piece{move.kind, move.color};
        draw_piece(PieceSpriteContext{piece, true, false, false, RestKind::Short, progress}, draw_x,
                   draw_y);
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
