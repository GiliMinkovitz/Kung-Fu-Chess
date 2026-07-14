#include "logic/game_state.h"
#include "model/game_config.h"
#include "test_helpers.h"
#include "ui/board_view_builder.h"

#include <doctest/doctest.h>

TEST_CASE("BoardViewBuilderTest - EmptyBoard") {
    kfc::GameState state(kfc::BoardModel{});
    const kfc::BoardViewModel view = kfc::BoardViewBuilder::build(state);

    CHECK_EQ(view.rows, 0u);
    CHECK_EQ(view.cols, 0u);
    CHECK(view.tokens.empty());
    CHECK_FALSE(view.selection.has_value());
    CHECK(view.animations.moves.empty());
    CHECK(view.animations.jumps.empty());
}

TEST_CASE("BoardViewBuilderTest - MapsDimensionsTokensAndClock") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    state.add_clock(250);

    const kfc::BoardViewModel view = kfc::BoardViewBuilder::build(state);

    CHECK_EQ(view.rows, 1u);
    CHECK_EQ(view.cols, 3u);
    CHECK_EQ(view.clock_ms, 250);
    CHECK_FALSE(view.game_over);
    REQUIRE_EQ(view.tokens.size(), 3u);
    CHECK_EQ(view.tokens[0], "wK");
    CHECK_EQ(view.tokens[1], ".");
    CHECK_EQ(view.tokens[2], "bK");
    CHECK_EQ(kfc::board_view_token_at(view, 0, 0), "wK");
    CHECK_EQ(kfc::board_view_token_at(view, 0, 2), "bK");
}

TEST_CASE("BoardViewBuilderTest - SelectionIncludedWhenPresent") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    state.select(0, 2);

    const kfc::BoardViewModel view = kfc::BoardViewBuilder::build(state);

    REQUIRE(view.selection.has_value());
    CHECK_EQ(view.selection->first, 0u);
    CHECK_EQ(view.selection->second, 2u);
}

TEST_CASE("BoardViewBuilderTest - SelectionOmittedWhenCleared") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    state.select(0, 0);
    state.clear_selection();

    const kfc::BoardViewModel view = kfc::BoardViewBuilder::build(state);

    CHECK_FALSE(view.selection.has_value());
}

TEST_CASE("BoardViewBuilderTest - ActiveMoveAnimationDuringTransit") {
    kfc::GameState state(kfc::test::make_board({{"wR", ".", "."}}));
    state.select(0, 0);
    state.move_selected_to(0, 2);
    state.add_clock(1000);

    const kfc::BoardViewModel view = kfc::BoardViewBuilder::build(state);

    REQUIRE_EQ(view.animations.moves.size(), 1u);
    const kfc::ActiveMoveSnapshot& move = view.animations.moves.front();
    CHECK_EQ(move.from_row, 0u);
    CHECK_EQ(move.from_col, 0u);
    CHECK_EQ(move.to_row, 0u);
    CHECK_EQ(move.to_col, 2u);
    CHECK(move.progress > 0.0f);
    CHECK(move.progress < 1.0f);
    CHECK(kfc::board_view_is_move_origin(view, 0, 0));
    CHECK_FALSE(kfc::board_view_is_move_origin(view, 0, 2));
}

TEST_CASE("BoardViewBuilderTest - ActiveJumpAnimationDuringTransit") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    state.select(0, 0);
    state.jump_selected();
    state.add_clock(500);

    const kfc::BoardViewModel view = kfc::BoardViewBuilder::build(state);

    REQUIRE_EQ(view.animations.jumps.size(), 1u);
    const kfc::ActiveJumpSnapshot& jump = view.animations.jumps.front();
    CHECK_EQ(jump.row, 0u);
    CHECK_EQ(jump.col, 0u);
    CHECK(jump.progress > 0.0f);
    CHECK(jump.progress < 1.0f);
    CHECK(kfc::board_view_is_jumping_cell(view, 0, 0));
    CHECK(kfc::board_view_jump_progress_at(view, 0, 0) == doctest::Approx(jump.progress));
}

TEST_CASE("BoardViewBuilderTest - GameOverFlagPropagated") {
    kfc::GameState state(kfc::test::make_board({{"wR", ".", "bK"}}));
    state.select(0, 0);
    state.move_selected_to(0, 2);
    state.add_clock(2000);

    const kfc::BoardViewModel view = kfc::BoardViewBuilder::build(state);

    CHECK(view.game_over);
    CHECK(view.animations.moves.empty());
}
