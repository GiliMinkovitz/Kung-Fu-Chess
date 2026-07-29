#include "model/game_config.h"
#include "model/piece.h"
#include "server/room/game_player.h"
#include "server/room/room_manager.h"
#include "test_helpers.h"

#include <doctest/doctest.h>

TEST_CASE("RoomManagerTest - CreatesAndFindsRooms") {
    kfc::RoomManager manager{kfc::test::make_board({{"wK", ".", "bK"}})};

    const kfc::RoomId first_id = manager.create_room();
    const kfc::RoomId second_id = manager.create_room();

    CHECK(first_id != second_id);

    kfc::Room* first = manager.find_room(first_id);
    kfc::Room* second = manager.find_room(second_id);
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    CHECK_EQ(first->id(), first_id);
    CHECK_EQ(second->id(), second_id);
    CHECK_FALSE(first->active());
    CHECK_FALSE(second->active());
    CHECK(manager.find_room(999) == nullptr);
}

TEST_CASE("RoomManagerTest - RemovesRooms") {
    kfc::RoomManager manager{kfc::test::make_board({{"wK", ".", "bK"}})};

    const kfc::RoomId room_id = manager.create_room();
    REQUIRE(manager.find_room(room_id) != nullptr);

    manager.remove_room(room_id);
    CHECK(manager.find_room(room_id) == nullptr);
}

TEST_CASE("RoomManagerTest - RemovesInactiveRooms") {
    kfc::RoomManager manager{kfc::test::make_board({{"wK", ".", "bK"}})};

    const kfc::RoomId active_id = manager.create_room();
    const kfc::RoomId inactive_id = manager.create_room();

    kfc::GamePlayer white{1, kfc::PieceColor::White, 1};
    kfc::GamePlayer black{2, kfc::PieceColor::Black, 2};
    manager.find_room(active_id)->activate(white, black);

    manager.remove_inactive_rooms();

    CHECK(manager.find_room(active_id) != nullptr);
    CHECK(manager.find_room(inactive_id) == nullptr);
}

TEST_CASE("RoomManagerTest - MultipleRoomsTickIndependently") {
    kfc::RoomManager manager{kfc::test::make_board({{"wK", ".", "bK"}})};

    const kfc::RoomId room_a_id = manager.create_room();
    const kfc::RoomId room_b_id = manager.create_room();

    kfc::Room* room_a = manager.find_room(room_a_id);
    kfc::Room* room_b = manager.find_room(room_b_id);
    REQUIRE(room_a != nullptr);
    REQUIRE(room_b != nullptr);

    room_a->tick(100);
    room_b->tick(500);

    CHECK_EQ(room_a->match().state().clock_ms(), 100);
    CHECK_EQ(room_b->match().state().clock_ms(), 500);

    manager.tick_all(50);

    CHECK_EQ(room_a->match().state().clock_ms(), 100);
    CHECK_EQ(room_b->match().state().clock_ms(), 500);
}

TEST_CASE("RoomManagerTest - TickAllAdvancesActiveRooms") {
    kfc::RoomManager manager{kfc::test::make_board({{"wK", ".", "bK"}})};

    const kfc::RoomId room_id = manager.create_room();
    kfc::GamePlayer white{1, kfc::PieceColor::White, 1};
    kfc::GamePlayer black{2, kfc::PieceColor::Black, 2};
    manager.find_room(room_id)->activate(white, black);

    manager.tick_all(200);

    CHECK_EQ(manager.find_room(room_id)->match().state().clock_ms(), 200);
    CHECK_EQ(manager.active_rooms().size(), 1u);
}
