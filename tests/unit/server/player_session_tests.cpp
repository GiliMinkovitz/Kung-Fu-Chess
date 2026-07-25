#include "database/player_repository.h"
#include "database/sqlite_database.h"
#include "network/websocket_client.h"
#include "server/player_session.h"
#include "server/websocket_server.h"

#include <doctest/doctest.h>

#include <chrono>
#include <memory>
#include <sqlite3.h>
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

struct SessionFixture {
    kfc::WebSocketServer server{0};
    std::unique_ptr<kfc::WebSocketClient> client;
    kfc::SqliteDatabase db{":memory:"};
    kfc::PlayerRepository repo{db};

    SessionFixture() {
        REQUIRE(db.open());
        REQUIRE(db.initialize_schema());
    }

    kfc::PlayerSession make_session() {
        client = std::make_unique<kfc::WebSocketClient>("127.0.0.1", server.port());
        std::thread accept_thread{[&]() { accept_one_client(server); }};
        client->connect();
        accept_thread.join();
        return kfc::PlayerSession{0, &server.clients().back()};
    }
};

}  // namespace

TEST_CASE("PlayerSessionTest - LogsInAndLoadsExistingPlayer") {
    SessionFixture fixture;
    REQUIRE(fixture.repo.create_player("existing", 1100).has_value());

    kfc::PlayerSession session = fixture.make_session();
    CHECK(session.login("existing", fixture.repo));
    CHECK(session.has_player());
    CHECK_EQ(session.player().username(), "existing");
    CHECK_EQ(session.player().rating(), 1100);
}

TEST_CASE("PlayerSessionTest - CreatesPlayerOnFirstLogin") {
    SessionFixture fixture;
    kfc::PlayerSession session = fixture.make_session();
    CHECK_EQ(session.id(), 0u);

    CHECK(session.login("new_player", fixture.repo));
    CHECK_EQ(session.player().username(), "new_player");
    CHECK_EQ(session.player().rating(), 1000);
}

TEST_CASE("PlayerSessionTest - RejectsInvalidLoginAttempts") {
    SessionFixture fixture;
    kfc::PlayerSession session = fixture.make_session();

    CHECK_FALSE(session.login("", fixture.repo));
    CHECK_FALSE(session.has_player());

    CHECK(session.login("valid", fixture.repo));
    CHECK_FALSE(session.login("other", fixture.repo));
}

TEST_CASE("PlayerSessionTest - ManagesSearchAndPlayStates") {
    SessionFixture fixture;
    kfc::PlayerSession session = fixture.make_session();
    REQUIRE(session.login("player", fixture.repo));

    CHECK_EQ(session.state(), kfc::PlayerSessionState::Connected);
    session.request_play();
    CHECK_EQ(session.state(), kfc::PlayerSessionState::Searching);

    session.cancel_search();
    CHECK_EQ(session.state(), kfc::PlayerSessionState::Connected);

    session.request_play();
    session.set_playing();
    CHECK_EQ(session.state(), kfc::PlayerSessionState::Playing);
}

TEST_CASE("PlayerSessionTest - ManagesAssignedSide") {
    SessionFixture fixture;
    kfc::PlayerSession session = fixture.make_session();
    REQUIRE(session.login("player", fixture.repo));

    CHECK_FALSE(session.has_side());
    session.set_side(kfc::PieceColor::White);
    CHECK(session.has_side());
    CHECK_EQ(session.side(), kfc::PieceColor::White);

    session.clear_side();
    CHECK_FALSE(session.has_side());
}

TEST_CASE("PlayerSessionTest - ThrowsWhenPlayerCreationFails") {
    const char* path = "kfc_readonly_player_session_test.db";
    {
        kfc::SqliteDatabase setup(path);
        REQUIRE(setup.open());
        REQUIRE(setup.initialize_schema());
    }

    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};
    kfc::SqliteDatabase db(path);
    REQUIRE(db.open());
    kfc::PlayerRepository repo{db};
    sqlite3* connection = db.connection();
    REQUIRE(connection != nullptr);
    REQUIRE(sqlite3_exec(connection, "PRAGMA query_only = ON;", nullptr, nullptr, nullptr) ==
            SQLITE_OK);

    std::thread accept_thread{[&]() { accept_one_client(server); }};
    client.connect();
    accept_thread.join();

    kfc::PlayerSession session{0, &server.clients().back()};
    CHECK_THROWS_AS(session.login("cannot_create", repo), std::runtime_error);
    std::remove(path);
}
