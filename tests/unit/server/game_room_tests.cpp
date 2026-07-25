#include "database/game_repository.h"
#include "database/player_repository.h"
#include "database/sqlite_database.h"
#include "network/websocket_client.h"
#include "server/game_room.h"
#include "server/player_session.h"
#include "server/websocket_server.h"
#include "test_helpers.h"

#include <doctest/doctest.h>

#include <chrono>
#include <memory>
#include <thread>

namespace {

void accept_one_client(kfc::WebSocketServer& server) {
    for (int attempt = 0; attempt < 1000 && server.clients().empty(); ++attempt) {
        server.try_accept();
        if (server.clients().empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

struct SharedDbFixture {
    kfc::SqliteDatabase db{":memory:"};
    kfc::PlayerRepository player_repo{db};
    kfc::GameRepository game_repo{db};

    SharedDbFixture() {
        REQUIRE(db.open());
        REQUIRE(db.initialize_schema());
    }
};

struct SessionFixture {
    kfc::WebSocketServer server{0};
    std::unique_ptr<kfc::WebSocketClient> client;
    std::unique_ptr<kfc::PlayerSession> session;

    SessionFixture(SharedDbFixture& shared, const std::string& username, int rating) {
        REQUIRE(shared.player_repo.create_player(username, rating).has_value());

        client = std::make_unique<kfc::WebSocketClient>("127.0.0.1", server.port());
        std::thread accept_thread{[&]() { accept_one_client(server); }};
        client->connect();
        accept_thread.join();

        session = std::make_unique<kfc::PlayerSession>(0, &server.clients().back());
        REQUIRE(session->login(username, shared.player_repo));
    }
};

}  // namespace

TEST_CASE("GameRoomTest - StartsInactive") {
    kfc::GameRoom room{kfc::test::make_board({{"wK", ".", "bK"}})};

    CHECK_FALSE(room.active());
    CHECK_FALSE(room.db_game_id().has_value());
    CHECK_FALSE(room.contains(nullptr));
    CHECK(room.white_session() == nullptr);
    CHECK(room.black_session() == nullptr);
    CHECK(room.white_player() == nullptr);
    CHECK(room.black_player() == nullptr);
}

TEST_CASE("GameRoomTest - ActivatesAndTracksPlayers") {
    SharedDbFixture shared;
    SessionFixture white_fixture{shared, "white_room", 1000};
    SessionFixture black_fixture{shared, "black_room", 1000};
    kfc::GameRoom room{kfc::test::make_board({{"wK", ".", "bK"}})};

    room.activate(white_fixture.session.get(), black_fixture.session.get(), shared.game_repo);

    CHECK(room.active());
    REQUIRE(room.db_game_id().has_value());
    CHECK(*room.db_game_id() > 0);

    CHECK(room.contains(white_fixture.session.get()));
    CHECK(room.contains(black_fixture.session.get()));
    CHECK_FALSE(room.contains(nullptr));

    CHECK(room.white_session() == white_fixture.session.get());
    CHECK(room.black_session() == black_fixture.session.get());
    CHECK_EQ(room.white_player()->username(), "white_room");
    CHECK_EQ(room.black_player()->username(), "black_room");

    const kfc::GameRoom& const_room = room;
    CHECK(const_room.white_session() == white_fixture.session.get());
    CHECK(const_room.black_session() == black_fixture.session.get());
    CHECK_EQ(const_room.white_player()->username(), "white_room");
    CHECK_EQ(const_room.black_player()->username(), "black_room");
    CHECK_FALSE(const_room.match().is_game_over());
    CHECK(white_fixture.session->has_side());
    CHECK(black_fixture.session->has_side());
    CHECK_EQ(white_fixture.session->side(), kfc::PieceColor::White);
    CHECK_EQ(black_fixture.session->side(), kfc::PieceColor::Black);
    CHECK_FALSE(room.match().is_game_over());
}

TEST_CASE("GameRoomTest - ResetClearsActiveState") {
    SharedDbFixture shared;
    SessionFixture white_fixture{shared, "reset_white", 1000};
    SessionFixture black_fixture{shared, "reset_black", 1000};
    kfc::GameRoom room{kfc::test::make_board({{"wK", ".", "bK"}})};

    room.activate(white_fixture.session.get(), black_fixture.session.get(), shared.game_repo);
    room.reset();

    CHECK_FALSE(room.active());
    CHECK_FALSE(room.db_game_id().has_value());
    CHECK(room.white_session() == nullptr);
    CHECK(room.black_session() == nullptr);
    CHECK_FALSE(white_fixture.session->has_side());
    CHECK_FALSE(black_fixture.session->has_side());
    CHECK_FALSE(room.match().is_game_over());
}
