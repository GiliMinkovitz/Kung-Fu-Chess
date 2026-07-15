#pragma once

#include <algorithm>
#include <cstddef>

namespace kfc {

inline constexpr int kReferenceCellSizePixels = 100;
inline constexpr double kReferencePieceFontScale = 1.2;
inline constexpr int kReferencePieceFontThickness = 2;
inline constexpr int kReferenceSelectionBorderThickness = 3;
inline constexpr int kReferenceJumpBorderThickness = 2;
inline constexpr double kReferenceGameOverFontScale = 1.0;
inline constexpr int kReferenceGameOverFontThickness = 2;
inline constexpr int kReferenceGameOverMarginX = 20;
inline constexpr int kReferenceGameOverMarginY = 40;

struct BoardLayout {
    int board_x = 0;
    int board_y = 0;
    int cell_size = 0;
    int board_width_pixels = 0;
    int board_height_pixels = 0;

    [[nodiscard]] int cell_origin_x(std::size_t col) const noexcept {
        return board_x + static_cast<int>(col) * cell_size;
    }

    [[nodiscard]] int cell_origin_y(std::size_t row) const noexcept {
        return board_y + static_cast<int>(row) * cell_size;
    }

    [[nodiscard]] int piece_text_x(int cell_x) const noexcept { return cell_x + cell_size / 6; }

    [[nodiscard]] int piece_text_y(int cell_y) const noexcept {
        return cell_y + (cell_size * 2) / 3;
    }

    [[nodiscard]] double piece_font_scale() const noexcept {
        return scaled_font_size(kReferencePieceFontScale);
    }

    [[nodiscard]] int piece_font_thickness() const noexcept {
        return scaled_size(kReferencePieceFontThickness);
    }

    [[nodiscard]] int selection_border_thickness() const noexcept {
        return scaled_size(kReferenceSelectionBorderThickness);
    }

    [[nodiscard]] int jump_border_thickness() const noexcept {
        return scaled_size(kReferenceJumpBorderThickness);
    }

    [[nodiscard]] int jump_lift_pixels(float lift_ratio) const noexcept {
        return static_cast<int>(static_cast<float>(cell_size) * lift_ratio);
    }

    [[nodiscard]] int game_over_text_x() const noexcept { return scaled_size(kReferenceGameOverMarginX); }

    [[nodiscard]] int game_over_text_y() const noexcept { return scaled_size(kReferenceGameOverMarginY); }

    [[nodiscard]] double game_over_font_scale() const noexcept {
        return scaled_font_size(kReferenceGameOverFontScale);
    }

    [[nodiscard]] int game_over_font_thickness() const noexcept {
        return scaled_size(kReferenceGameOverFontThickness);
    }

    [[nodiscard]] bool try_pixel_to_cell(int x, int y, std::size_t width_cells,
                                         std::size_t height_cells, std::size_t& row,
                                         std::size_t& col) const noexcept {
        if (cell_size <= 0 || width_cells == 0 || height_cells == 0) {
            return false;
        }

        if (x < board_x || y < board_y) {
            return false;
        }

        const int local_x = x - board_x;
        const int local_y = y - board_y;
        if (local_x >= board_width_pixels || local_y >= board_height_pixels) {
            return false;
        }

        col = static_cast<std::size_t>(local_x / cell_size);
        row = static_cast<std::size_t>(local_y / cell_size);
        return col < width_cells && row < height_cells;
    }

private:
    [[nodiscard]] double scaled_font_size(double reference_scale) const noexcept {
        if (cell_size <= 0) {
            return reference_scale;
        }

        return reference_scale * static_cast<double>(cell_size) / kReferenceCellSizePixels;
    }

    [[nodiscard]] int scaled_size(int reference_pixels) const noexcept {
        if (cell_size <= 0) {
            return reference_pixels;
        }

        return std::max(1, reference_pixels * cell_size / kReferenceCellSizePixels);
    }
};

}  // namespace kfc
