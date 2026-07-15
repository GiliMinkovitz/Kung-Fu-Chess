#include "ui/layout/board_layout_calculator.h"

#include <doctest/doctest.h>

namespace {

const kfc::BoardLayoutCalculator kCalculator;

void check_board_fits_in_window(const kfc::BoardLayout& layout, int window_width,
                                int window_height) {
    CHECK(layout.board_x >= 0);
    CHECK(layout.board_y >= 0);
    CHECK(layout.board_x + layout.board_width_pixels <= window_width);
    CHECK(layout.board_y + layout.board_height_pixels <= window_height);
}

void check_cells_are_square(const kfc::BoardLayout& layout, int board_width_cells,
                            int board_height_cells) {
    CHECK_EQ(layout.board_width_pixels, layout.cell_size * board_width_cells);
    CHECK_EQ(layout.board_height_pixels, layout.cell_size * board_height_cells);
}

void check_centered_horizontally(const kfc::BoardLayout& layout, int window_width) {
    const int leftover = window_width - layout.board_width_pixels;
    CHECK_EQ(layout.board_x, leftover / 2);
    CHECK_EQ(layout.board_x + layout.board_width_pixels, window_width - (leftover - leftover / 2));
}

void check_centered_vertically(const kfc::BoardLayout& layout, int window_height) {
    const int leftover = window_height - layout.board_height_pixels;
    CHECK_EQ(layout.board_y, leftover / 2);
    CHECK_EQ(layout.board_y + layout.board_height_pixels,
             window_height - (leftover - leftover / 2));
}

}  // namespace

TEST_CASE("BoardLayoutCalculatorTest - SquareBoard") {
    constexpr int window_width = 800;
    constexpr int window_height = 800;
    constexpr int board_width_cells = 8;
    constexpr int board_height_cells = 8;

    const kfc::BoardLayout layout =
        kCalculator.calculate(window_width, window_height, board_width_cells, board_height_cells);

    CHECK_EQ(layout.cell_size, 100);
    CHECK_EQ(layout.board_width_pixels, 800);
    CHECK_EQ(layout.board_height_pixels, 800);
    CHECK_EQ(layout.board_x, 0);
    CHECK_EQ(layout.board_y, 0);
    check_cells_are_square(layout, board_width_cells, board_height_cells);
    check_board_fits_in_window(layout, window_width, window_height);
}

TEST_CASE("BoardLayoutCalculatorTest - RectangularBoard") {
    constexpr int window_width = 1000;
    constexpr int window_height = 500;
    constexpr int board_width_cells = 10;
    constexpr int board_height_cells = 5;

    const kfc::BoardLayout layout =
        kCalculator.calculate(window_width, window_height, board_width_cells, board_height_cells);

    CHECK_EQ(layout.cell_size, 100);
    CHECK_EQ(layout.board_width_pixels, 1000);
    CHECK_EQ(layout.board_height_pixels, 500);
    CHECK_EQ(layout.board_x, 0);
    CHECK_EQ(layout.board_y, 0);
    check_cells_are_square(layout, board_width_cells, board_height_cells);
    check_board_fits_in_window(layout, window_width, window_height);
    check_centered_horizontally(layout, window_width);
    check_centered_vertically(layout, window_height);
}

TEST_CASE("BoardLayoutCalculatorTest - SmallWindow") {
    constexpr int window_width = 100;
    constexpr int window_height = 80;
    constexpr int board_width_cells = 8;
    constexpr int board_height_cells = 8;

    const kfc::BoardLayout layout =
        kCalculator.calculate(window_width, window_height, board_width_cells, board_height_cells);

    CHECK_EQ(layout.cell_size, 10);
    CHECK_EQ(layout.board_width_pixels, 80);
    CHECK_EQ(layout.board_height_pixels, 80);
    CHECK_EQ(layout.board_x, 10);
    CHECK_EQ(layout.board_y, 0);
    check_cells_are_square(layout, board_width_cells, board_height_cells);
    check_board_fits_in_window(layout, window_width, window_height);
}

TEST_CASE("BoardLayoutCalculatorTest - LargeWindow") {
    constexpr int window_width = 3840;
    constexpr int window_height = 2160;
    constexpr int board_width_cells = 12;
    constexpr int board_height_cells = 8;

    const kfc::BoardLayout layout =
        kCalculator.calculate(window_width, window_height, board_width_cells, board_height_cells);

    CHECK_EQ(layout.cell_size, 270);
    CHECK_EQ(layout.board_width_pixels, 3240);
    CHECK_EQ(layout.board_height_pixels, 2160);
    CHECK_EQ(layout.board_x, 300);
    CHECK_EQ(layout.board_y, 0);
    check_cells_are_square(layout, board_width_cells, board_height_cells);
    check_board_fits_in_window(layout, window_width, window_height);
    check_centered_horizontally(layout, window_width);
    check_centered_vertically(layout, window_height);
}

TEST_CASE("BoardLayoutCalculatorTest - CenteringBehavior") {
    constexpr int window_width = 641;
    constexpr int window_height = 479;
    constexpr int board_width_cells = 8;
    constexpr int board_height_cells = 8;

    const kfc::BoardLayout layout =
        kCalculator.calculate(window_width, window_height, board_width_cells, board_height_cells);

    CHECK_EQ(layout.cell_size, 59);
    CHECK_EQ(layout.board_width_pixels, 472);
    CHECK_EQ(layout.board_height_pixels, 472);
    CHECK_EQ(layout.board_x, 84);
    CHECK_EQ(layout.board_y, 3);
    check_centered_horizontally(layout, window_width);
    check_centered_vertically(layout, window_height);
    check_board_fits_in_window(layout, window_width, window_height);
}

TEST_CASE("BoardLayoutCalculatorTest - BoardAlwaysFitsInsideWindow") {
    const int window_sizes[][2] = {{50, 50}, {127, 83}, {999, 333}, {1920, 1080}};
    const int board_sizes[][2] = {{1, 1}, {3, 7}, {8, 8}, {16, 9}, {20, 4}};

    for (const auto& window : window_sizes) {
        for (const auto& board : board_sizes) {
            const kfc::BoardLayout layout =
                kCalculator.calculate(window[0], window[1], board[0], board[1]);

            check_board_fits_in_window(layout, window[0], window[1]);
            check_cells_are_square(layout, board[0], board[1]);
            CHECK(layout.cell_size <= window[0] / board[0]);
            CHECK(layout.cell_size <= window[1] / board[1]);
        }
    }
}

TEST_CASE("BoardLayoutCalculatorTest - NonSquareBoardUsesTighterAxis") {
    constexpr int window_width = 640;
    constexpr int window_height = 480;
    constexpr int board_width_cells = 16;
    constexpr int board_height_cells = 4;

    const kfc::BoardLayout layout =
        kCalculator.calculate(window_width, window_height, board_width_cells, board_height_cells);

    CHECK_EQ(layout.cell_size, 40);
    CHECK_EQ(layout.board_width_pixels, 640);
    CHECK_EQ(layout.board_height_pixels, 160);
    CHECK_EQ(layout.board_x, 0);
    CHECK_EQ(layout.board_y, 160);
    check_board_fits_in_window(layout, window_width, window_height);
}

TEST_CASE("BoardLayoutCalculatorTest - InvalidInputsReturnEmptyLayout") {
    const kfc::BoardLayout layout =
        kCalculator.calculate(0, 100, 8, 8);

    CHECK_EQ(layout.board_x, 0);
    CHECK_EQ(layout.board_y, 0);
    CHECK_EQ(layout.cell_size, 0);
    CHECK_EQ(layout.board_width_pixels, 0);
    CHECK_EQ(layout.board_height_pixels, 0);
}

TEST_CASE("BoardLayoutTest - PixelToCellUsesBoardOffsets") {
    const kfc::BoardLayout layout = kCalculator.calculate(100, 80, 8, 8);

    std::size_t row = 0;
    std::size_t col = 0;

    CHECK(layout.try_pixel_to_cell(10, 0, 8, 8, row, col));
    CHECK_EQ(row, 0u);
    CHECK_EQ(col, 0u);

    CHECK(layout.try_pixel_to_cell(19, 9, 8, 8, row, col));
    CHECK_EQ(row, 0u);
    CHECK_EQ(col, 0u);

    CHECK_FALSE(layout.try_pixel_to_cell(9, 0, 8, 8, row, col));
    CHECK_FALSE(layout.try_pixel_to_cell(90, 80, 8, 8, row, col));
}

TEST_CASE("BoardLayoutTest - PixelToCellMatchesLegacyOrigin") {
    const kfc::BoardLayout layout = kCalculator.calculate(300, 100, 3, 1);

    std::size_t row = 0;
    std::size_t col = 0;

    CHECK(layout.try_pixel_to_cell(50, 50, 3, 1, row, col));
    CHECK_EQ(row, 0u);
    CHECK_EQ(col, 0u);

    CHECK(layout.try_pixel_to_cell(150, 50, 3, 1, row, col));
    CHECK_EQ(row, 0u);
    CHECK_EQ(col, 1u);

    CHECK_FALSE(layout.try_pixel_to_cell(350, 50, 3, 1, row, col));
}

TEST_CASE("BoardLayoutTest - ScalesTypographyAndChromeFromCellSize") {
    const kfc::BoardLayout layout = kCalculator.calculate(800, 800, 8, 8);

    CHECK_EQ(layout.piece_font_scale(), doctest::Approx(1.2));
    CHECK_EQ(layout.piece_font_thickness(), 2);
    CHECK_EQ(layout.selection_border_thickness(), 3);
    CHECK_EQ(layout.jump_border_thickness(), 2);
    CHECK_EQ(layout.game_over_text_x(), 20);
    CHECK_EQ(layout.game_over_text_y(), 40);
    CHECK_EQ(layout.game_over_font_scale(), doctest::Approx(1.0));
    CHECK_EQ(layout.game_over_font_thickness(), 2);
    CHECK_EQ(layout.jump_lift_pixels(0.25f), 25);
}

TEST_CASE("BoardLayoutTest - ScalesTypographyForSmallerCells") {
    const kfc::BoardLayout layout = kCalculator.calculate(100, 80, 8, 8);

    CHECK_EQ(layout.cell_size, 10);
    CHECK_EQ(layout.piece_font_thickness(), 1);
    CHECK_EQ(layout.selection_border_thickness(), 1);
    CHECK_EQ(layout.game_over_text_x(), 2);
    CHECK_EQ(layout.game_over_text_y(), 4);
    CHECK(layout.piece_font_scale() < 1.2);
}
