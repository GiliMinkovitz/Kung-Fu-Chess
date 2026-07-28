#include "network/websocket_client.h"
#include "server/game_server.h"
#include "test_game_server_fixture.h"
#include "test_game_server_helpers.h"
#include "test_helpers.h"

#include <doctest/doctest.h>

#include <chrono>
#include <string>
#include <thread>

namespace {

void connect_through_server(kfc::GameServer& server, kfc::WebSocketClient& client) {
    const std::size_t expected_count = server.websocket_server().clients().size() + 1;
    std::thread accept_thread{[&]() {
        for (int attempt = 0; attempt < 1000 && server.websocket_server().clients().size() < expected_count;
             ++attempt) {
            server.tick_once();
            if (server.websocket_server().clients().size() < expected_count) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }};
    client.connect();
    accept_thread.join();
    REQUIRE_EQ(server.websocket_server().clients().size(), expected_count);
}

std::optional<std::string> poll_login_message(kfc::WebSocketClient& client,
                                              int max_attempts = 200) {
    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        if (const auto message = client.try_receive_snapshot()) {
            if (message->find("login_") != std::string::npos) {
                return message;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return std::nullopt;
}

void login_client(kfc::GameServer& server, kfc::WebSocketClient& client,
                  const std::string& username) {
    connect_through_server(server, client);
    REQUIRE(client.try_send("login " + username + " testpass"));
    for (int attempt = 0; attempt < 50; ++attempt) {
        server.tick_once();
        if (poll_login_message(client).has_value()) {
            return;
        }
    }
    FAIL("Expected login_ok response");
}

void login_and_queue(kfc::GameServer& server, kfc::WebSocketClient& client,
                     const std::string& username) {
    login_client(server, client, username);
    REQUIRE(client.try_send("play"));
    server.tick_once();
}

void start_two_matches(kfc::GameServer& server, kfc::WebSocketClient& g1_white,
                       kfc::WebSocketClient& g1_black, kfc::WebSocketClient& g2_white,
                       kfc::WebSocketClient& g2_black) {
    login_and_queue(server, g1_white, "multi_g1_white");
    login_and_queue(server, g1_black, "multi_g1_black");

    for (int attempt = 0; attempt < 50 && server.room_manager().active_rooms().size() < 1u;
         ++attempt) {
        server.tick_once();
    }
    REQUIRE_EQ(server.room_manager().active_rooms().size(), 1u);

    login_and_queue(server, g2_white, "multi_g2_white");
    login_and_queue(server, g2_black, "multi_g2_black");

    for (int attempt = 0; attempt < 50 && server.room_manager().active_rooms().size() < 2u;
         ++attempt) {
        server.tick_once();
    }
    REQUIRE_EQ(server.room_manager().active_rooms().size(), 2u);
}

std::optional<std::string> drain_latest_snapshot(kfc::WebSocketClient& client) {
    std::optional<std::string> latest;
    for (int burst = 0; burst < 100; ++burst) {
        const auto message = client.try_receive_snapshot();
        if (!message) {
            break;
        }
        if (message->find("snapshot") != std::string::npos) {
            latest = std::move(message);
        }
    }
    return latest;
}

}  // namespace

TEST_CASE("GameServerMultiRoomTest - RoomsStoreSessionBindingsAndDbGameIds") {
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;
    kfc::WebSocketClient g1_white{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient g1_black{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient g2_white{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient g2_black{"127.0.0.1", server.websocket_server().port()};

    start_two_matches(server, g1_white, g1_black, g2_white, g2_black);

    kfc::Room* room1 = kfc::test::find_room_for_player(server, "multi_g1_white");
    kfc::Room* room2 = kfc::test::find_room_for_player(server, "multi_g2_white");
    REQUIRE(room1 != nullptr);
    REQUIRE(room2 != nullptr);

    REQUIRE(room1->white_session() != nullptr);
    REQUIRE(room1->black_session() != nullptr);
    REQUIRE(room2->white_session() != nullptr);
    REQUIRE(room2->black_session() != nullptr);
    REQUIRE(room1->db_game_id().has_value());
    REQUIRE(room2->db_game_id().has_value());
    CHECK_NE(*room1->db_game_id(), *room2->db_game_id());
    CHECK_EQ(room1->white_session()->player().username(), "multi_g1_white");
    CHECK_EQ(room1->black_session()->player().username(), "multi_g1_black");
    CHECK_EQ(room2->white_session()->player().username(), "multi_g2_white");
    CHECK_EQ(room2->black_session()->player().username(), "multi_g2_black");
}

TEST_CASE("GameServerMultiRoomTest - TwoRoomsExistSimultaneously") {
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;
    kfc::WebSocketClient g1_white{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient g1_black{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient g2_white{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient g2_black{"127.0.0.1", server.websocket_server().port()};

    start_two_matches(server, g1_white, g1_black, g2_white, g2_black);

    CHECK_EQ(server.room_manager().active_rooms().size(), 2u);
}

TEST_CASE("GameServerMultiRoomTest - PlayersAssignedToCorrectRoom") {
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;
    kfc::WebSocketClient g1_white{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient g1_black{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient g2_white{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient g2_black{"127.0.0.1", server.websocket_server().port()};

    start_two_matches(server, g1_white, g1_black, g2_white, g2_black);

    kfc::Room* g1_white_room = kfc::test::find_room_for_player(server, "multi_g1_white");
    kfc::Room* g1_black_room = kfc::test::find_room_for_player(server, "multi_g1_black");
    kfc::Room* g2_white_room = kfc::test::find_room_for_player(server, "multi_g2_white");
    kfc::Room* g2_black_room = kfc::test::find_room_for_player(server, "multi_g2_black");

    REQUIRE(g1_white_room != nullptr);
    REQUIRE(g1_black_room != nullptr);
    REQUIRE(g2_white_room != nullptr);
    REQUIRE(g2_black_room != nullptr);

    CHECK_EQ(g1_white_room->id(), g1_black_room->id());
    CHECK_EQ(g2_white_room->id(), g2_black_room->id());
    CHECK_NE(g1_white_room->id(), g2_white_room->id());
}

TEST_CASE("GameServerMultiRoomTest - CommandsRoutedToCorrectRoom") {
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;
    kfc::WebSocketClient g1_white{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient g1_black{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient g2_white{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient g2_black{"127.0.0.1", server.websocket_server().port()};

    start_two_matches(server, g1_white, g1_black, g2_white, g2_black);

    kfc::Room* room1 = kfc::test::find_room_for_player(server, "multi_g1_white");
    kfc::Room* room2 = kfc::test::find_room_for_player(server, "multi_g2_white");
    REQUIRE(room1 != nullptr);
    REQUIRE(room2 != nullptr);

    REQUIRE(g1_white.try_send("select 0 0"));
    server.tick_once();

    CHECK(room1->match().state().has_selection());
    CHECK_FALSE(room2->match().state().has_selection());
}

TEST_CASE("GameServerMultiRoomTest - SnapshotsIsolatedBetweenRooms") {
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;
    kfc::WebSocketClient g1_white{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient g1_black{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient g2_white{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient g2_black{"127.0.0.1", server.websocket_server().port()};

    start_two_matches(server, g1_white, g1_black, g2_white, g2_black);

    REQUIRE(g1_white.try_send("select 0 0"));
    server.tick_once();

    for (int attempt = 0; attempt < 100; ++attempt) {
        server.tick_once();
        const auto g1_snapshot = drain_latest_snapshot(g1_white);
        const auto g2_snapshot = drain_latest_snapshot(g2_white);
        if (g1_snapshot.has_value() && g2_snapshot.has_value()) {
            CHECK_NE(*g1_snapshot, *g2_snapshot);
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    FAIL("Expected distinct snapshots for isolated rooms");
}

TEST_CASE("GameServerMultiRoomTest - RoomsTickIndependently") {
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;
    kfc::WebSocketClient g1_white{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient g1_black{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient g2_white{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient g2_black{"127.0.0.1", server.websocket_server().port()};

    start_two_matches(server, g1_white, g1_black, g2_white, g2_black);

    kfc::Room* room1 = kfc::test::find_room_for_player(server, "multi_g1_white");
    kfc::Room* room2 = kfc::test::find_room_for_player(server, "multi_g2_white");
    REQUIRE(room1 != nullptr);
    REQUIRE(room2 != nullptr);

    const std::int64_t room1_clock_before = room1->match().state().clock_ms();
    const std::int64_t room2_clock_before = room2->match().state().clock_ms();

    room1->tick(100);
    CHECK_EQ(room1->match().state().clock_ms(), room1_clock_before + 100);
    CHECK_EQ(room2->match().state().clock_ms(), room2_clock_before);

    room2->tick(250);
    CHECK_EQ(room1->match().state().clock_ms(), room1_clock_before + 100);
    CHECK_EQ(room2->match().state().clock_ms(), room2_clock_before + 250);
}
