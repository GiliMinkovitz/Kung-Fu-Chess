#include "ui/view/board_view_model.h"

#include <doctest/doctest.h>

namespace {

kfc::BoardViewModel make_view(std::size_t height, std::size_t width) {
    kfc::BoardViewModel view;
    view.height = height;
    view.width = width;
    view.cells.resize(height * width);
    return view;
}

}  // namespace

TEST_CASE("BoardViewModelTest - CellIndexComputesRowMajorOrder") {
    CHECK_EQ(kfc::board_view_cell_index(1, 2, 4), 6u);
}

TEST_CASE("BoardViewModelTest - CellAtReturnsEmptyForOutOfBounds") {
    const kfc::BoardViewModel view = make_view(2, 2);

    CHECK_FALSE(kfc::board_view_piece_at(view, 2, 0).has_value());
    CHECK_FALSE(kfc::board_view_piece_at(view, 0, 2).has_value());
}

TEST_CASE("BoardViewModelTest - CellAtReturnsEmptyWhenCellsTooSmall") {
    kfc::BoardViewModel view;
    view.height = 2;
    view.width = 2;
    view.cells.resize(1);

    CHECK_FALSE(kfc::board_view_piece_at(view, 1, 1).has_value());
}

TEST_CASE("BoardViewModelTest - AnimationQueriesReturnDefaultsWhenMissing") {
    const kfc::BoardViewModel view = make_view(1, 1);

    CHECK_FALSE(kfc::board_view_is_move_origin(view, 0, 0));
    CHECK_FALSE(kfc::board_view_is_jump_origin(view, 0, 0));
    CHECK_FALSE(kfc::board_view_is_jumping_cell(view, 0, 0));
    CHECK(kfc::board_view_jump_progress_at(view, 0, 0) == doctest::Approx(0.0f));
    CHECK_FALSE(kfc::board_view_is_resting_cell(view, 0, 0));
    CHECK(kfc::board_view_rest_progress_at(view, 0, 0) == doctest::Approx(0.0f));
    CHECK(kfc::board_view_rest_kind_at(view, 0, 0) == kfc::RestKind::Short);
}

TEST_CASE("BoardViewModelTest - AnimationQueriesMatchSnapshots") {
    kfc::BoardViewModel view = make_view(2, 2);
    view.animations.moves.push_back({1, kfc::PieceKind::Rook, kfc::PieceColor::White, 0, 0, 0, 1,
                                     0.5f});
    view.animations.jumps.push_back({2, kfc::PieceKind::King, kfc::PieceColor::Black, 1, 1, 0.75f});
    view.animations.rests.push_back({3, 0, 1, kfc::RestKind::Long, 0.25f});

    CHECK(kfc::board_view_is_move_origin(view, 0, 0));
    CHECK(kfc::board_view_is_jump_origin(view, 1, 1));
    CHECK(kfc::board_view_is_jumping_cell(view, 1, 1));
    CHECK(kfc::board_view_jump_progress_at(view, 1, 1) == doctest::Approx(0.75f));
    CHECK(kfc::board_view_is_resting_cell(view, 0, 1));
    CHECK(kfc::board_view_rest_progress_at(view, 0, 1) == doctest::Approx(0.25f));
    CHECK(kfc::board_view_rest_kind_at(view, 0, 1) == kfc::RestKind::Long);
}
