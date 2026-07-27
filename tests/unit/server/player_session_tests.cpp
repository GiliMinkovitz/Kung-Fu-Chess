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
    const auto existing = fixture.repo.find_by_username("existing");
    REQUIRE(existing.has_value());
    session.assign_user(static_cast<kfc::UserId>(existing->id()), existing->username(),
                        existing->rating());

    CHECK(session.has_user());
    CHECK(session.has_player());
    CHECK_EQ(session.user_id(), static_cast<kfc::UserId>(existing->id()));
    CHECK_EQ(session.player().username(), "existing");
    CHECK_EQ(session.rating(), 1100);
}

TEST_CASE("PlayerSessionTest - BindsNewPlayer") {
    SessionFixture fixture;
    kfc::PlayerSession session = fixture.make_session();
    CHECK_EQ(session.id(), 0u);

    session.assign_user(1, "new_player", 1000);
    CHECK_EQ(session.user_id(), 1u);
    CHECK_EQ(session.player().username(), "new_player");
    CHECK_EQ(session.rating(), 1000);
}

TEST_CASE("PlayerSessionTest - RefreshesBoundPlayer") {
    SessionFixture fixture;
    kfc::PlayerSession session = fixture.make_session();

    session.assign_user(1, "player", 1000);
    session.assign_user(1, "player", 1013);
    CHECK_EQ(session.rating(), 1013);
}

TEST_CASE("PlayerSessionTest - ClearsUserIdentity") {
    SessionFixture fixture;
    kfc::PlayerSession session = fixture.make_session();

    session.assign_user(1, "player", 1000);
    CHECK(session.has_user());

    session.clear_user();
    CHECK_FALSE(session.has_user());
    CHECK_FALSE(session.has_player());
}

TEST_CASE("PlayerSessionTest - TwoSessionsLinkToDifferentUsers") {
    SessionFixture fixture;
    kfc::PlayerSession first = fixture.make_session();
    kfc::PlayerSession second = fixture.make_session();

    first.assign_user(10, "first_user", 1000);
    second.assign_user(20, "second_user", 1100);

    CHECK_NE(first.user_id(), second.user_id());
    CHECK_EQ(first.player().username(), "first_user");
    CHECK_EQ(second.player().username(), "second_user");
}

TEST_CASE("PlayerSessionTest - BindPlayerDelegatesToUserIdentity") {
    SessionFixture fixture;
    kfc::PlayerSession session = fixture.make_session();

    session.bind_player(kfc::Player{5, "legacy_player", 1200});
    CHECK(session.has_user());
    CHECK_EQ(session.user_id(), 5u);
    CHECK_EQ(session.rating(), 1200);
}

TEST_CASE("PlayerSessionTest - ManagesSearchAndPlayStates") {
    SessionFixture fixture;
    kfc::PlayerSession session = fixture.make_session();
    session.assign_user(1, "player", 1000);

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
    session.assign_user(1, "player", 1000);

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
    session.assign_user(1, "player", 1000);

    CHECK_FALSE(session.has_room());
    session.assign_room(42);
    CHECK(session.has_room());
    CHECK_EQ(session.room_id(), 42u);

    session.clear_room();
    CHECK_FALSE(session.has_room());
}
