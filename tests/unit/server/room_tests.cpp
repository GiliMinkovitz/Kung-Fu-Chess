#include "server/room/game_player.h"
#include "server/room/room.h"
#include "test_helpers.h"

#include <doctest/doctest.h>

#include <string>

namespace {

kfc::GamePlayer make_game_player(kfc::UserId user_id, kfc::PieceColor side, kfc::PlayerId player_id) {
    return kfc::GamePlayer{user_id, side, player_id};
}

}  // namespace

TEST_CASE("RoomTest - StartsInactive") {
    kfc::Room room{1, kfc::test::make_board({{"wK", ".", "bK"}})};

    CHECK_EQ(room.id(), 1u);
    CHECK_FALSE(room.active());
    CHECK_FALSE(room.contains_player(99));
    CHECK(room.white_player() == nullptr);
    CHECK(room.black_player() == nullptr);
    CHECK_FALSE(room.is_game_over());
}

TEST_CASE("RoomTest - ActivatesAndTracksPlayers") {
    const kfc::GamePlayer white = make_game_player(1, kfc::PieceColor::White, 10);
    const kfc::GamePlayer black = make_game_player(2, kfc::PieceColor::Black, 20);
    kfc::Room room{1, kfc::test::make_board({{"wK", ".", "bK"}})};

    room.activate(white, black);

    CHECK(room.active());
    CHECK(room.contains_player(1));
    CHECK(room.contains_player(2));
    CHECK_FALSE(room.contains_player(99));
    REQUIRE(room.white_player() != nullptr);
    REQUIRE(room.black_player() != nullptr);
    CHECK_EQ(room.white_player()->user_id, 1u);
    CHECK_EQ(room.black_player()->user_id, 2u);
    CHECK_EQ(room.white_player()->side, kfc::PieceColor::White);
    CHECK_EQ(room.black_player()->side, kfc::PieceColor::Black);
    CHECK_FALSE(room.match().is_game_over());
}

TEST_CASE("RoomTest - ResetClearsActiveState") {
    const kfc::GamePlayer white = make_game_player(1, kfc::PieceColor::White, 10);
    const kfc::GamePlayer black = make_game_player(2, kfc::PieceColor::Black, 20);
    kfc::Room room{1, kfc::test::make_board({{"wK", ".", "bK"}})};

    room.activate(white, black);
    room.reset();

    CHECK_FALSE(room.active());
    CHECK(room.white_player() == nullptr);
    CHECK(room.black_player() == nullptr);
    CHECK_FALSE(room.match().is_game_over());
}

TEST_CASE("RoomTest - StoresDbGameId") {
    kfc::Room room{1, kfc::test::make_board({{"wK", ".", "bK"}})};

    CHECK_FALSE(room.db_game_id().has_value());

    room.set_db_game_id(42);

    REQUIRE(room.db_game_id().has_value());
    CHECK_EQ(*room.db_game_id(), 42);
}

TEST_CASE("RoomTest - ResetClearsPlayerMetadata") {
    const kfc::GamePlayer white = make_game_player(1, kfc::PieceColor::White, 10);
    const kfc::GamePlayer black = make_game_player(2, kfc::PieceColor::Black, 20);
    kfc::Room room{1, kfc::test::make_board({{"wK", ".", "bK"}})};

    room.activate(white, black);
    room.set_db_game_id(99);
    room.reset();

    CHECK(room.white_player() == nullptr);
    CHECK(room.black_player() == nullptr);
    CHECK_FALSE(room.db_game_id().has_value());
}

TEST_CASE("RoomTest - TicksAndGeneratesSnapshot") {
    kfc::Room room{1, kfc::test::make_board({{"wK", ".", "bK"}})};

    room.tick(250);
    CHECK_EQ(room.match().state().clock_ms(), 250);

    const std::string snapshot = room.generate_snapshot();
    CHECK_FALSE(snapshot.empty());
    CHECK(snapshot.find("snapshot") != std::string::npos);
}

TEST_CASE("RoomTest - SubmitsActions") {
    kfc::Room room{1, kfc::test::make_board({{"wK", ".", "bK"}})};

    room.submit_action(kfc::Select{0, 0});
    CHECK(room.match().state().has_selection());
}
