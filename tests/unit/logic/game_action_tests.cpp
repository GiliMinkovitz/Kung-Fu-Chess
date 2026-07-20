#include "logic/game_action.h"
#include "logic/game_state.h"
#include "test_helpers.h"

#include <doctest/doctest.h>

namespace {

bool observable_state_matches(const kfc::GameState& expected, const kfc::GameState& actual) {
    if (expected.clock_ms() != actual.clock_ms()) {
        return false;
    }
    if (expected.has_selection() != actual.has_selection()) {
        return false;
    }
    if (expected.is_game_over() != actual.is_game_over()) {
        return false;
    }
    if (!expected.same_board_layout_as(actual)) {
        return false;
    }

    std::size_t expected_row = 0;
    std::size_t expected_col = 0;
    std::size_t actual_row = 0;
    std::size_t actual_col = 0;
    const bool expected_has = expected.selection(expected_row, expected_col);
    const bool actual_has = actual.selection(actual_row, actual_col);
    if (expected_has != actual_has) {
        return false;
    }
    if (expected_has && (expected_row != actual_row || expected_col != actual_col)) {
        return false;
    }

    for (std::size_t row = 0; row < expected.rows(); ++row) {
        for (std::size_t col = 0; col < expected.cols(); ++col) {
            if (expected.is_piece_moving(row, col) != actual.is_piece_moving(row, col)) {
                return false;
            }
            if (expected.is_piece_jumping(row, col) != actual.is_piece_jumping(row, col)) {
                return false;
            }
            if (expected.is_piece_resting(row, col) != actual.is_piece_resting(row, col)) {
                return false;
            }
        }
    }

    return true;
}

}  // namespace

TEST_CASE("GameActionTest - SelectMatchesDirectCall") {
    const kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "bK"}});
    kfc::GameState via_action(board);
    kfc::GameState via_direct(board);

    via_action.apply_action(kfc::Select{0, 0});
    via_direct.select(0, 0);

    CHECK(observable_state_matches(via_direct, via_action));
}

TEST_CASE("GameActionTest - ClearSelectionMatchesDirectCall") {
    const kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "bK"}});
    kfc::GameState via_action(board);
    kfc::GameState via_direct(board);

    via_action.apply_action(kfc::Select{0, 0});
    via_direct.select(0, 0);
    via_action.apply_action(kfc::ClearSelection{});
    via_direct.clear_selection();

    CHECK(observable_state_matches(via_direct, via_action));
}

TEST_CASE("GameActionTest - MoveSelectedMatchesDirectCall") {
    const kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "."}});
    kfc::GameState via_action(board);
    kfc::GameState via_direct(board);

    via_action.apply_action(kfc::Select{0, 0});
    via_direct.select(0, 0);
    via_action.apply_action(kfc::MoveSelected{0, 2});
    via_direct.move_selected_to(0, 2);

    CHECK(observable_state_matches(via_direct, via_action));
}

TEST_CASE("GameActionTest - JumpAtMatchesDirectCall") {
    const kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "."}});
    kfc::GameState via_action(board);
    kfc::GameState via_direct(board);

    via_action.apply_action(kfc::JumpAt{0, 0});
    via_direct.jump_at(0, 0);

    CHECK(observable_state_matches(via_direct, via_action));
}

TEST_CASE("GameActionTest - JumpSelectedMatchesDirectCall") {
    const kfc::BoardModel board = kfc::test::make_board({{"wR", ".", "."}});
    kfc::GameState via_action(board);
    kfc::GameState via_direct(board);

    via_action.apply_action(kfc::Select{0, 0});
    via_direct.select(0, 0);
    via_action.apply_action(kfc::JumpSelected{});
    via_direct.jump_selected();

    CHECK(observable_state_matches(via_direct, via_action));
}

TEST_CASE("GameActionTest - AdvanceClockMatchesDirectCall") {
    const kfc::BoardModel board = kfc::test::make_board({{"wK", ".", "bK"}});
    kfc::GameState via_action(board);
    kfc::GameState via_direct(board);

    via_action.apply_action(kfc::AdvanceClock{250});
    via_direct.add_clock(250);
    via_action.apply_action(kfc::AdvanceClock{50});
    via_direct.add_clock(50);

    CHECK(observable_state_matches(via_direct, via_action));
}

TEST_CASE("GameActionTest - ActionSequenceMatchesDirectCalls") {
    const kfc::BoardModel board =
        kfc::test::make_board({{".", ".", "."}, {"wR", ".", "."}, {"bR", ".", "."}});
    kfc::GameState via_action(board);
    kfc::GameState via_direct(board);

    via_action.apply_action(kfc::Select{2, 0});
    via_direct.select(2, 0);
    via_action.apply_action(kfc::MoveSelected{1, 0});
    via_direct.move_selected_to(1, 0);
    via_action.apply_action(kfc::AdvanceClock{500});
    via_direct.add_clock(500);
    via_action.apply_action(kfc::Select{1, 0});
    via_direct.select(1, 0);
    via_action.apply_action(kfc::JumpSelected{});
    via_direct.jump_selected();
    via_action.apply_action(kfc::AdvanceClock{500});
    via_direct.add_clock(500);

    CHECK(observable_state_matches(via_direct, via_action));
}
