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

    REQUIRE_EQ(actual.animations.moves.size(), expected.animations.moves.size());
    for (std::size_t index = 0; index < expected.animations.moves.size(); ++index) {
        const kfc::ActiveMoveSnapshot& expected_move = expected.animations.moves[index];
        const kfc::ActiveMoveSnapshot& actual_move = actual.animations.moves[index];
        CHECK_EQ(actual_move.piece_id, expected_move.piece_id);
        CHECK(actual_move.kind == expected_move.kind);
        CHECK(actual_move.color == expected_move.color);
        CHECK_EQ(actual_move.from_row, expected_move.from_row);
        CHECK_EQ(actual_move.from_col, expected_move.from_col);
        CHECK_EQ(actual_move.to_row, expected_move.to_row);
        CHECK_EQ(actual_move.to_col, expected_move.to_col);
        CHECK(actual_move.progress == doctest::Approx(expected_move.progress));
    }

    REQUIRE_EQ(actual.animations.jumps.size(), expected.animations.jumps.size());
    for (std::size_t index = 0; index < expected.animations.jumps.size(); ++index) {
        const kfc::ActiveJumpSnapshot& expected_jump = expected.animations.jumps[index];
        const kfc::ActiveJumpSnapshot& actual_jump = actual.animations.jumps[index];
        CHECK_EQ(actual_jump.piece_id, expected_jump.piece_id);
        CHECK(actual_jump.kind == expected_jump.kind);
        CHECK(actual_jump.color == expected_jump.color);
        CHECK_EQ(actual_jump.row, expected_jump.row);
        CHECK_EQ(actual_jump.col, expected_jump.col);
        CHECK(actual_jump.progress == doctest::Approx(expected_jump.progress));
    }

    REQUIRE_EQ(actual.animations.rests.size(), expected.animations.rests.size());
    for (std::size_t index = 0; index < expected.animations.rests.size(); ++index) {
        const kfc::ActiveRestSnapshot& expected_rest = expected.animations.rests[index];
        const kfc::ActiveRestSnapshot& actual_rest = actual.animations.rests[index];
        CHECK_EQ(actual_rest.piece_id, expected_rest.piece_id);
        CHECK_EQ(actual_rest.row, expected_rest.row);
        CHECK_EQ(actual_rest.col, expected_rest.col);
        CHECK(actual_rest.kind == expected_rest.kind);
        CHECK(actual_rest.progress == doctest::Approx(expected_rest.progress));
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
    CHECK(parsed->animations.moves.empty());
    CHECK(parsed->animations.jumps.empty());
    CHECK(parsed->animations.rests.empty());
}

TEST_CASE("SnapshotReaderTest - RoundTripPreservesAnimations") {
    kfc::BoardViewModel original;
    original.height = 2;
    original.width = 2;
    original.clock_ms = 100;
    original.cells = {{}, {}, {}, {}};
    original.animations.moves.push_back(
        {1, kfc::PieceKind::Rook, kfc::PieceColor::White, 0, 0, 0, 1, 0.5f});
    original.animations.jumps.push_back(
        {2, kfc::PieceKind::King, kfc::PieceColor::Black, 1, 1, 0.75f});
    original.animations.rests.push_back({3, 0, 1, kfc::RestKind::Long, 0.25f});

    const std::optional<kfc::BoardViewModel> parsed =
        kfc::read_snapshot(kfc::write_snapshot(original));

    REQUIRE(parsed.has_value());
    check_view_matches(original, *parsed);
}

TEST_CASE("SnapshotReaderTest - RoundTripShortRestAnimation") {
    kfc::BoardViewModel original;
    original.height = 1;
    original.width = 1;
    original.clock_ms = 7;
    original.cells = {{}};
    original.animations.rests.push_back({4, 0, 0, kfc::RestKind::Short, 0.125f});

    const std::optional<kfc::BoardViewModel> parsed =
        kfc::read_snapshot(kfc::write_snapshot(original));

    REQUIRE(parsed.has_value());
    check_view_matches(original, *parsed);
}

TEST_CASE("SnapshotReaderTest - RejectsInvalidSnapshot") {
    CHECK_FALSE(kfc::read_snapshot("").has_value());
    CHECK_FALSE(kfc::read_snapshot("not_a_snapshot\n").has_value());
    CHECK_FALSE(kfc::read_snapshot("snapshot\nclock_ms bad\n").has_value());
    CHECK_FALSE(
        kfc::read_snapshot("snapshot\nclock_ms 1\ngame_over false\nheight 1\nwidth 2\ncells\nwK")
            .has_value());
}

TEST_CASE("SnapshotReaderTest - RejectsMalformedHeaderFields") {
    CHECK_FALSE(kfc::read_snapshot("snapshot\n").has_value());
    CHECK_FALSE(kfc::read_snapshot("snapshot\nclock_ms 1 extra\n").has_value());
    CHECK_FALSE(kfc::read_snapshot("snapshot\nclock_ms -1\ngame_over false\nheight 1\nwidth 1\ncells\nwK")
            .has_value());
    CHECK_FALSE(kfc::read_snapshot("snapshot\nclock_ms 1\ngame_over maybe\nheight 1\nwidth 1\ncells\nwK")
            .has_value());
    CHECK_FALSE(kfc::read_snapshot("snapshot\nclock_ms 1\ngame_over false\nheight 1\nwidth 1\ncells extra\nwK")
            .has_value());
    CHECK_FALSE(kfc::read_snapshot("snapshot\nclock_ms 1\ngame_over false\nheight bad\nwidth 1\ncells\nwK")
            .has_value());
    CHECK_FALSE(kfc::read_snapshot("snapshot\nclock_ms 1\ngame_over false\nheight 1\nwidth bad\ncells\nwK")
            .has_value());
    CHECK_FALSE(kfc::read_snapshot("snapshot\nclock_ms 1\ngame_over false\nselection 0\ncells\nwK")
            .has_value());
    CHECK_FALSE(kfc::read_snapshot("snapshot\nclock_ms 1\ngame_over false\nselection bad 0\ncells\nwK")
            .has_value());
    CHECK_FALSE(kfc::read_snapshot("snapshot\nclock_ms 1\ngame_over false\nunknown 1\ncells\nwK")
            .has_value());
}

TEST_CASE("SnapshotReaderTest - RejectsMalformedCellsAndAnimations") {
    CHECK_FALSE(kfc::read_snapshot("snapshot\nclock_ms 1\ngame_over false\nheight 1\nwidth 1\ncells\n")
            .has_value());
    CHECK_FALSE(kfc::read_snapshot("snapshot\nclock_ms 1\ngame_over false\nheight 1\nwidth 1\ncells\nbad")
            .has_value());
    CHECK_FALSE(kfc::read_snapshot("snapshot\nclock_ms 1\ngame_over false\nheight 1\nwidth 2\ncells\nwK")
            .has_value());
    CHECK_FALSE(kfc::read_snapshot(
                    "snapshot\nclock_ms 1\ngame_over false\nheight 1\nwidth 1\ncells\nwK\n"
                    "animations\nmoves\nbad")
            .has_value());
    CHECK_FALSE(kfc::read_snapshot(
                    "snapshot\nclock_ms 1\ngame_over false\nheight 1\nwidth 1\ncells\n.\n"
                    "animations\nmoves\nmove bad\njumps\nrests\n")
            .has_value());
    CHECK_FALSE(kfc::read_snapshot(
                    "snapshot\nclock_ms 1\ngame_over false\nheight 1\nwidth 1\ncells\n.\n"
                    "animations\nmoves\njumps\njump bad\nrests\n")
            .has_value());
    CHECK_FALSE(kfc::read_snapshot(
                    "snapshot\nclock_ms 1\ngame_over false\nheight 1\nwidth 1\ncells\n.\n"
                    "animations\nmoves\njumps\nrests\nrest bad")
            .has_value());
}

TEST_CASE("SnapshotReaderTest - ParsesCarriageReturnLinesAndShortRest") {
    const std::string text =
        "snapshot\r\nclock_ms 5\ngame_over false\r\nheight 1\nwidth 1\ncells\n.\n"
        "animations\nmoves\njumps\nrests\nrest piece_id 1 row 0 0 kind short progress 0.500000\n";
    const std::optional<kfc::BoardViewModel> parsed = kfc::read_snapshot(text);

    REQUIRE(parsed.has_value());
    REQUIRE_EQ(parsed->animations.rests.size(), 1u);
    CHECK(parsed->animations.rests.front().kind == kfc::RestKind::Short);
}

TEST_CASE("SnapshotReaderTest - RejectsMalformedAnimationDetails") {
    CHECK_FALSE(kfc::read_snapshot(
                    "snapshot\nclock_ms 1\ngame_over false\nheight 1\nwidth 1\ncells\n.\n"
                    "animations\nmoves\nmove piece_id -1 piece wR from 0 0 to 0 1 progress 0.5\n"
                    "jumps\nrests\n")
            .has_value());
    CHECK_FALSE(kfc::read_snapshot(
                    "snapshot\nclock_ms 1\ngame_over false\nheight 1\nwidth 1\ncells\n.\n"
                    "animations\nmoves\nmove piece_id 1 piece wR from 0 0 to 0 1 progress bad\n"
                    "jumps\nrests\n")
            .has_value());
    CHECK_FALSE(kfc::read_snapshot(
                    "snapshot\nclock_ms 1\ngame_over false\nheight 1\nwidth 1\ncells\n.\n"
                    "animations\nmoves\njumps\njump piece_id -1 piece wK row 0 0 progress 0.5\n"
                    "rests\n")
            .has_value());
    CHECK_FALSE(kfc::read_snapshot(
                    "snapshot\nclock_ms 1\ngame_over false\nheight 1\nwidth 1\ncells\n.\n"
                    "animations\nmoves\njumps\nrests\nrest piece_id 1 row 0 0 kind bad progress 0.5\n")
            .has_value());
    CHECK_FALSE(kfc::read_snapshot(
                    "snapshot\nclock_ms 1\ngame_over false\nheight 1\nwidth 1\ncells\n.\n"
                    "animations\nmoves\njumps\nrests\nrest piece_id 1 row 0 0 kind short progress bad\n")
            .has_value());
    CHECK_FALSE(kfc::read_snapshot(
                    "snapshot\nclock_ms 1\ngame_over false\nheight 1\nwidth 1\ncells\n.\n"
                    "animations\nbad\n")
            .has_value());
    CHECK_FALSE(kfc::read_snapshot(
                    "snapshot\nclock_ms 1\ngame_over false\nheight 1\nwidth 1\ncells extra\nwK")
            .has_value());
    CHECK_FALSE(kfc::read_snapshot(
                    "snapshot\nclock_ms 1\ngame_over false\nheight 1\nwidth 1\nselection 0\n"
                    "cells\nwK")
            .has_value());
}

TEST_CASE("SnapshotReaderTest - IgnoresBlankLinesInHeaderAndAnimations") {
    const std::optional<kfc::BoardViewModel> with_header_blank = kfc::read_snapshot(
        "snapshot\n\nclock_ms 1\ngame_over false\nheight 1\nwidth 1\ncells\nwK");
    REQUIRE(with_header_blank.has_value());
    CHECK_EQ(with_header_blank->clock_ms, 1);

    const std::optional<kfc::BoardViewModel> with_animation_blank = kfc::read_snapshot(
        "snapshot\nclock_ms 1\ngame_over false\nheight 1\nwidth 1\ncells\n.\n"
        "animations\nmoves\n\njumps\nrests\n");
    REQUIRE(with_animation_blank.has_value());
    CHECK(with_animation_blank->animations.moves.empty());
}

TEST_CASE("SnapshotReaderTest - ParserHelpersRejectEmptyTokens") {
    CHECK_FALSE(kfc::test::snapshot_parse_int64_for_tests("").has_value());
    CHECK_FALSE(kfc::test::snapshot_parse_float_for_tests("").has_value());
}

TEST_CASE("SnapshotReaderTest - RejectsMalformedHeaderTokenCounts") {
    CHECK_FALSE(kfc::read_snapshot(
                    "snapshot\nclock_ms 1\ngame_over false extra\nheight 1\nwidth 1\ncells\n.")
            .has_value());
    CHECK_FALSE(kfc::read_snapshot(
                    "snapshot\nclock_ms 1\ngame_over false\nheight 1 extra\nwidth 1\ncells\n.")
            .has_value());
    CHECK_FALSE(kfc::read_snapshot(
                    "snapshot\nclock_ms 1\ngame_over false\nheight 1\nwidth 1 extra\ncells\n.")
            .has_value());
}

TEST_CASE("SnapshotReaderTest - RejectsMissingAnimationsSection") {
    CHECK_FALSE(kfc::read_snapshot(
                    "snapshot\nclock_ms 1\ngame_over false\nheight 1\nwidth 1\ncells\n.\n"
                    "not_animations\n")
            .has_value());
}

TEST_CASE("SnapshotReaderTest - RoundTripInvalidRestKindUsesFallbackToken") {
    kfc::BoardViewModel original;
    original.height = 1;
    original.width = 1;
    original.cells = {{}};
    original.animations.rests.push_back(
        {5, 0, 0, static_cast<kfc::RestKind>(99), 0.5f});

    const std::string text = kfc::write_snapshot(original);
    CHECK(text.find("kind short") != std::string::npos);
}
