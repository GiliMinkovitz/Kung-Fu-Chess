#include "app/matchmaking_config.h"
#include "database/player_repository.h"
#include "database/sqlite_database.h"
#include "model/piece.h"
#include "network/websocket_client.h"
#include "server/matchmaking/match_created_handler.h"
#include "server/matchmaking/matchmaking_service.h"
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
        session.bind_player(*repo_.find_by_username(username));
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

class RecordingMatchHandler : public kfc::IMatchCreatedHandler {
public:
    kfc::RoomId create_match(kfc::PlayerSession* white, kfc::PlayerSession* black) override {
        white->set_playing();
        black->set_playing();
        white->set_side(kfc::PieceColor::White);
        black->set_side(kfc::PieceColor::Black);

        created_matches_.push_back({white, black});
        const kfc::RoomId room_id = next_room_id_++;
        white->assign_room(room_id);
        black->assign_room(room_id);
        return room_id;
    }

    [[nodiscard]] const std::vector<std::pair<kfc::PlayerSession*, kfc::PlayerSession*>>&
    created_matches() const noexcept {
        return created_matches_;
    }

private:
    kfc::RoomId next_room_id_ = 1;
    std::vector<std::pair<kfc::PlayerSession*, kfc::PlayerSession*>> created_matches_;
};

kfc::MatchmakingService make_test_service(kfc::IMatchCreatedHandler& handler) {
    return kfc::MatchmakingService{handler, kfc::app::MatchmakingConfig{}};
}

}  // namespace

TEST_CASE("MatchmakingServiceTest - PlayerEntersQueue") {
    TestSessionFactory factory;
    RecordingMatchHandler handler;
    kfc::MatchmakingService service = make_test_service(handler);
    const auto now = std::chrono::steady_clock::now();

    kfc::PlayerSession& first = factory.create("queue_alice", 1000);
    CHECK_FALSE(service.enqueue(first, now).has_value());
    CHECK_EQ(service.waiting_count(), 1u);
    CHECK(handler.created_matches().empty());
}

TEST_CASE("MatchmakingServiceTest - TwoPlayersCreateMatch") {
    TestSessionFactory factory;
    RecordingMatchHandler handler;
    kfc::MatchmakingService service = make_test_service(handler);
    const auto now = std::chrono::steady_clock::now();

    kfc::PlayerSession& first = factory.create("match_alice", 1000);
    CHECK_FALSE(service.enqueue(first, now).has_value());

    kfc::PlayerSession& second = factory.create("match_bob", 1050);
    const auto match = service.enqueue(second, now);

    REQUIRE(match.has_value());
    CHECK_EQ(match->white, &first);
    CHECK_EQ(match->black, &second);
    CHECK_EQ(service.waiting_count(), 0u);
    REQUIRE_EQ(handler.created_matches().size(), 1u);
    CHECK_EQ(handler.created_matches()[0].first, &first);
    CHECK_EQ(handler.created_matches()[0].second, &second);
}

TEST_CASE("MatchmakingServiceTest - MatchedPlayersReceiveSameRoomId") {
    TestSessionFactory factory;
    RecordingMatchHandler handler;
    kfc::MatchmakingService service = make_test_service(handler);
    const auto now = std::chrono::steady_clock::now();

    kfc::PlayerSession& first = factory.create("room_alice", 1000);
    CHECK_FALSE(service.enqueue(first, now).has_value());

    kfc::PlayerSession& second = factory.create("room_bob", 1000);
    const auto match = service.enqueue(second, now);
    REQUIRE(match.has_value());

    CHECK(first.has_room());
    CHECK(second.has_room());
    CHECK_EQ(first.room_id(), match->room_id);
    CHECK_EQ(second.room_id(), match->room_id);
    CHECK_EQ(first.room_id(), 1u);
    CHECK(first.has_side());
    CHECK(second.has_side());
    CHECK_EQ(first.state(), kfc::PlayerSessionState::Playing);
    CHECK_EQ(second.state(), kfc::PlayerSessionState::Playing);
}

TEST_CASE("MatchmakingServiceTest - DifferentMatchesCreateDifferentRooms") {
    TestSessionFactory factory;
    RecordingMatchHandler handler;
    kfc::MatchmakingService service = make_test_service(handler);
    const auto now = std::chrono::steady_clock::now();

    kfc::PlayerSession& g1_white = factory.create("g1_white", 1000);
    kfc::PlayerSession& g1_black = factory.create("g1_black", 1000);
    kfc::PlayerSession& g2_white = factory.create("g2_white", 1000);
    kfc::PlayerSession& g2_black = factory.create("g2_black", 1000);

    CHECK_FALSE(service.enqueue(g1_white, now).has_value());
    const auto match1 = service.enqueue(g1_black, now);
    REQUIRE(match1.has_value());

    CHECK_FALSE(service.enqueue(g2_white, now).has_value());
    const auto match2 = service.enqueue(g2_black, now);
    REQUIRE(match2.has_value());

    CHECK_NE(match1->room_id, match2->room_id);
    CHECK_EQ(g1_white.room_id(), g1_black.room_id());
    CHECK_EQ(g2_white.room_id(), g2_black.room_id());
    CHECK_EQ(handler.created_matches().size(), 2u);
}

TEST_CASE("MatchmakingServiceTest - RemovesPlayerFromQueue") {
    TestSessionFactory factory;
    RecordingMatchHandler handler;
    kfc::MatchmakingService service = make_test_service(handler);
    const auto now = std::chrono::steady_clock::now();

    kfc::PlayerSession& waiting = factory.create("remove_alice", 1000);
    CHECK_FALSE(service.enqueue(waiting, now).has_value());
    CHECK_EQ(service.waiting_count(), 1u);

    service.remove(waiting);
    CHECK_EQ(service.waiting_count(), 0u);
}

TEST_CASE("MatchmakingServiceTest - ReportsQueueTimeouts") {
    TestSessionFactory factory;
    RecordingMatchHandler handler;
    kfc::MatchmakingService service = make_test_service(handler);
    const auto now = std::chrono::steady_clock::now();

    kfc::PlayerSession& waiting = factory.create("timeout_alice", 1000);
    CHECK_FALSE(service.enqueue(waiting, now - std::chrono::seconds(60)).has_value());

    const auto timed_out = service.check_timeouts(now);
    REQUIRE_EQ(timed_out.size(), 1u);
    CHECK_EQ(timed_out.front(), &waiting);
    CHECK_EQ(service.waiting_count(), 0u);
}
