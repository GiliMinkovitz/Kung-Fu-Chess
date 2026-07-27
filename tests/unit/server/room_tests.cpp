#include "server/player.h"
#include "server/room/room.h"
#include "test_helpers.h"

#include <doctest/doctest.h>

#include <string>

TEST_CASE("RoomTest - StartsInactive") {
    kfc::Room room{1, kfc::test::make_board({{"wK", ".", "bK"}})};

    CHECK_EQ(room.id(), 1u);
    CHECK_FALSE(room.active());
    CHECK_FALSE(room.contains_player(nullptr));
    CHECK(room.white_player() == nullptr);
    CHECK(room.black_player() == nullptr);
    CHECK_FALSE(room.is_game_over());
}

TEST_CASE("RoomTest - ActivatesAndTracksPlayers") {
    kfc::Player white{1, "white_room", 1000};
    kfc::Player black{2, "black_room", 1000};
    kfc::Room room{1, kfc::test::make_board({{"wK", ".", "bK"}})};

    room.activate(&white, &black);

    CHECK(room.active());
    CHECK(room.contains_player(&white));
    CHECK(room.contains_player(&black));
    CHECK_FALSE(room.contains_player(nullptr));
    CHECK(room.white_player() == &white);
    CHECK(room.black_player() == &black);
    CHECK_EQ(room.white_player()->username(), "white_room");
    CHECK_EQ(room.black_player()->username(), "black_room");
    CHECK_FALSE(room.match().is_game_over());
}

TEST_CASE("RoomTest - ResetClearsActiveState") {
    kfc::Player white{1, "reset_white", 1000};
    kfc::Player black{2, "reset_black", 1000};
    kfc::Room room{1, kfc::test::make_board({{"wK", ".", "bK"}})};

    room.activate(&white, &black);
    room.reset();

    CHECK_FALSE(room.active());
    CHECK(room.white_player() == nullptr);
    CHECK(room.black_player() == nullptr);
    CHECK_FALSE(room.match().is_game_over());
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
