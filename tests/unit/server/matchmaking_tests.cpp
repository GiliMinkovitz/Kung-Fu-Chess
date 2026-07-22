#include "database/player_repository.h"
#include "database/sqlite_database.h"
#include "network/websocket_client.h"
#include "server/matchmaking.h"
#include "server/player_session.h"
#include "server/websocket_server.h"

#include <doctest/doctest.h>

#include <chrono>
#include <list>
#include <memory>
#include <thread>
#include <vector>

namespace {

void accept_one_client(kfc::WebSocketServer& server) {
    for (int attempt = 0; attempt < 1000 && server.clients().empty(); ++attempt) {
        server.try_accept();
        if (server.clients().empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

struct OwnedTestSession {
    kfc::WebSocketServer server{0};
    std::unique_ptr<kfc::WebSocketClient> client;
    std::unique_ptr<kfc::PlayerSession> session;
};

class TestSessionFactory {
public:
    TestSessionFactory()
        : db_(":memory:"),
          repo_(db_) {
        REQUIRE(db_.open());
        REQUIRE(db_.initialize_schema());
    }

    kfc::PlayerSession& create(const std::string& username, int rating) {
        REQUIRE(repo_.create_player(username, rating).has_value());

        auto owned = std::make_unique<OwnedTestSession>();
        owned->client = std::make_unique<kfc::WebSocketClient>("127.0.0.1", owned->server.port());

        std::thread accept_thread{[&]() { accept_one_client(owned->server); }};
        owned->client->connect();
        accept_thread.join();

        owned->session = std::make_unique<kfc::PlayerSession>(next_id_++,
                                                              &owned->server.clients().back());
        kfc::PlayerSession& session = *owned->session;
        REQUIRE(session.login(username, repo_));
        session.request_play();
        sessions_.push_back(std::move(owned));
        return session;
    }

private:
    kfc::SqliteDatabase db_;
    kfc::PlayerRepository repo_;
    std::vector<std::unique_ptr<OwnedTestSession>> sessions_;
    std::size_t next_id_ = 0;
};

}  // namespace

TEST_CASE("MatchmakingTest - MatchesCompatiblePlayers") {
    TestSessionFactory factory;
    kfc::Matchmaking matchmaking;
    const auto now = std::chrono::steady_clock::now();

    kfc::PlayerSession& first = factory.create("alice", 1000);
    CHECK_FALSE(matchmaking.enqueue(first, now).has_value());
    CHECK_EQ(matchmaking.waiting_count(), 1);

    kfc::PlayerSession& second = factory.create("bob", 1050);
    const auto matched = matchmaking.enqueue(second, now);
    REQUIRE(matched.has_value());
    CHECK_EQ((*matched)[0], &first);
    CHECK_EQ((*matched)[1], &second);
    CHECK_EQ(matchmaking.waiting_count(), 0);
}

TEST_CASE("MatchmakingTest - SkipsIncompatiblePlayers") {
    TestSessionFactory factory;
    kfc::Matchmaking matchmaking;
    const auto now = std::chrono::steady_clock::now();

    kfc::PlayerSession& first = factory.create("alice", 1000);
    CHECK_FALSE(matchmaking.enqueue(first, now).has_value());

    kfc::PlayerSession& incompatible = factory.create("bob", 1200);
    CHECK_FALSE(matchmaking.enqueue(incompatible, now).has_value());
    CHECK_EQ(matchmaking.waiting_count(), 2);

    kfc::PlayerSession& compatible = factory.create("carol", 1040);
    const auto matched = matchmaking.enqueue(compatible, now);
    REQUIRE(matched.has_value());
    CHECK_EQ((*matched)[0], &first);
    CHECK_EQ((*matched)[1], &compatible);
    CHECK_EQ(matchmaking.waiting_count(), 1);
}

TEST_CASE("MatchmakingTest - RemovesTimedOutPlayers") {
    TestSessionFactory factory;
    kfc::Matchmaking matchmaking;
    const auto now = std::chrono::steady_clock::now();

    kfc::PlayerSession& waiting = factory.create("alice", 1000);
    CHECK_FALSE(matchmaking.enqueue(waiting, now - kfc::Matchmaking::kQueueTimeout).has_value());

    const auto timed_out = matchmaking.check_timeouts(now);
    REQUIRE_EQ(timed_out.size(), 1);
    CHECK_EQ(timed_out.front(), &waiting);
    CHECK_EQ(matchmaking.waiting_count(), 0);
}

TEST_CASE("MatchmakingTest - IgnoresDisconnectedPlayersWhenMatching") {
    TestSessionFactory factory;
    kfc::Matchmaking matchmaking;
    const auto now = std::chrono::steady_clock::now();

    kfc::PlayerSession& disconnected = factory.create("alice", 1000);
    CHECK_FALSE(matchmaking.enqueue(disconnected, now).has_value());
    disconnected.connection()->close();

    kfc::PlayerSession& second = factory.create("bob", 1000);
    CHECK_FALSE(matchmaking.enqueue(second, now).has_value());
    CHECK_EQ(matchmaking.waiting_count(), 2);
}
