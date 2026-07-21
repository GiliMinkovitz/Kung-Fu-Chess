#include "logic/game_state.h"
#include "network/snapshot_reader.h"
#include "server/snapshot_writer.h"
#include "test_helpers.h"
#include "ui/view/board_view_builder.h"
#include "ui/view/board_view_model.h"

#include <doctest/doctest.h>

namespace {

void check_view_matches(const kfc::BoardViewModel& expected, const kfc::BoardViewModel& actual) {
    CHECK_EQ(actual.height, expected.height);
    CHECK_EQ(actual.width, expected.width);
    CHECK_EQ(actual.clock_ms, expected.clock_ms);
    CHECK_EQ(actual.game_over, expected.game_over);
    CHECK_EQ(actual.selection.has_value(), expected.selection.has_value());
    if (expected.selection.has_value()) {
        REQUIRE(actual.selection.has_value());
        CHECK_EQ(actual.selection->first, expected.selection->first);
        CHECK_EQ(actual.selection->second, expected.selection->second);
    }

    REQUIRE_EQ(actual.cells.size(), expected.cells.size());
    for (std::size_t index = 0; index < expected.cells.size(); ++index) {
        const kfc::CellView& expected_cell = expected.cells[index];
        const kfc::CellView& actual_cell = actual.cells[index];
        CHECK_EQ(actual_cell.piece.has_value(), expected_cell.piece.has_value());
        if (expected_cell.piece.has_value()) {
            REQUIRE(actual_cell.piece.has_value());
            CHECK(actual_cell.piece->color == expected_cell.piece->color);
            CHECK(actual_cell.piece->kind == expected_cell.piece->kind);
        }
    }
}

}  // namespace

TEST_CASE("SnapshotReaderTest - RoundTripPreservesImportantFieldsFromBuilder") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    state.add_clock(250);
    state.select(0, 2);

    const kfc::BoardViewModel original = kfc::BoardViewBuilder::build(state);
    const std::string text = kfc::write_snapshot(original);
    const std::optional<kfc::BoardViewModel> parsed = kfc::read_snapshot(text);

    REQUIRE(parsed.has_value());
    check_view_matches(original, *parsed);
}

TEST_CASE("SnapshotReaderTest - RoundTripWithoutSelection") {
    kfc::GameState state(kfc::test::make_board({
        {"bR", "bN", "bB"},
        {"wP", ".", "wP"},
    }));
    state.add_clock(42);

    const kfc::BoardViewModel original = kfc::BoardViewBuilder::build(state);
    const std::optional<kfc::BoardViewModel> parsed =
        kfc::read_snapshot(kfc::write_snapshot(original));

    REQUIRE(parsed.has_value());
    check_view_matches(original, *parsed);
}

TEST_CASE("SnapshotReaderTest - RoundTripGameOverFlag") {
    kfc::GameState state(kfc::test::make_board({{"wR", ".", "bK"}}));
    state.select(0, 0);
    state.move_selected_to(0, 2);
    state.add_clock(2000);

    const kfc::BoardViewModel original = kfc::BoardViewBuilder::build(state);
    const std::optional<kfc::BoardViewModel> parsed =
        kfc::read_snapshot(kfc::write_snapshot(original));

    REQUIRE(parsed.has_value());
    CHECK(original.game_over);
    check_view_matches(original, *parsed);
}

TEST_CASE("SnapshotReaderTest - ParsesMinimalSnapshot") {
    const std::string text =
        "snapshot\nclock_ms 42\ngame_over false\nheight 1\nwidth 1\ncells\nwK";
    const std::optional<kfc::BoardViewModel> parsed = kfc::read_snapshot(text);

    REQUIRE(parsed.has_value());
    CHECK_EQ(parsed->height, 1u);
    CHECK_EQ(parsed->width, 1u);
    CHECK_EQ(parsed->clock_ms, 42);
    CHECK_FALSE(parsed->game_over);
    CHECK_FALSE(parsed->selection.has_value());
    REQUIRE_EQ(parsed->cells.size(), 1u);
    REQUIRE(parsed->cells[0].piece.has_value());
    CHECK(parsed->cells[0].piece->color == kfc::PieceColor::White);
    CHECK(parsed->cells[0].piece->kind == kfc::PieceKind::King);
}

TEST_CASE("SnapshotReaderTest - RejectsInvalidSnapshot") {
    CHECK_FALSE(kfc::read_snapshot("").has_value());
    CHECK_FALSE(kfc::read_snapshot("not_a_snapshot\n").has_value());
    CHECK_FALSE(kfc::read_snapshot("snapshot\nclock_ms bad\n").has_value());
    CHECK_FALSE(
        kfc::read_snapshot("snapshot\nclock_ms 1\ngame_over false\nheight 1\nwidth 2\ncells\nwK")
            .has_value());
}
