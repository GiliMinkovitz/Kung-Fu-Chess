#include "database/player_repository.h"
#include "database/sqlite_database.h"
#include "network/websocket_client.h"
#include "server/player_session.h"
#include "server/websocket_server.h"

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

TEST_CASE("PlayerSessionTest - BindsExistingPlayer") {
    SessionFixture fixture;
    REQUIRE(fixture.repo.create_player("existing", 1100).has_value());

    kfc::PlayerSession session = fixture.make_session();
    session.bind_player(*fixture.repo.find_by_username("existing"));
    CHECK(session.has_player());
    CHECK_EQ(session.player().username(), "existing");
    CHECK_EQ(session.player().rating(), 1100);
}

TEST_CASE("PlayerSessionTest - BindsNewPlayer") {
    SessionFixture fixture;
    kfc::PlayerSession session = fixture.make_session();
    CHECK_EQ(session.id(), 0u);

    session.bind_player(kfc::Player{1, "new_player", 1000});
    CHECK_EQ(session.player().username(), "new_player");
    CHECK_EQ(session.player().rating(), 1000);
}

TEST_CASE("PlayerSessionTest - RefreshesBoundPlayer") {
    SessionFixture fixture;
    kfc::PlayerSession session = fixture.make_session();

    session.bind_player(kfc::Player{1, "player", 1000});
    session.bind_player(kfc::Player{1, "player", 1013});
    CHECK_EQ(session.player().rating(), 1013);
}

TEST_CASE("PlayerSessionTest - ManagesSearchAndPlayStates") {
    SessionFixture fixture;
    kfc::PlayerSession session = fixture.make_session();
    session.bind_player(kfc::Player{1, "player", 1000});

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
    session.bind_player(kfc::Player{1, "player", 1000});

    CHECK_FALSE(session.has_side());
    session.set_side(kfc::PieceColor::White);
    CHECK(session.has_side());
    CHECK_EQ(session.side(), kfc::PieceColor::White);

    session.clear_side();
    CHECK_FALSE(session.has_side());
}

TEST_CASE("PlayerSessionTest - ManagesRoomAssignment") {
    SessionFixture fixture;
    kfc::PlayerSession session = fixture.make_session();
    session.bind_player(kfc::Player{1, "player", 1000});

    CHECK_FALSE(session.has_room());
    session.assign_room(42);
    CHECK(session.has_room());
    CHECK_EQ(session.room_id(), 42u);

    session.clear_room();
    CHECK_FALSE(session.has_room());
}
