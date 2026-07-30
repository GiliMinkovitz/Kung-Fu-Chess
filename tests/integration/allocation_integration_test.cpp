#include "app/in_memory_runtime_store.h"
#include "app/server_metrics.h"
#include "app/game_server_location.h"
#include "database/i_game_repository.h"
#include "model/piece.h"
#include "server/game/game_allocation_handler.h"
#include "server/game/game_creation_listener.h"
#include "server/game/local_game_host.h"
#include "server/game/remote_game_allocator.h"
#include "server/game/runtime_store_game_server_registry.h"
#include "server/gateway/game_redirect_info.h"
#include "server/gateway/i_game_gateway.h"
#include "server/match/match_lifecycle_handler.h"
#include "network/websocket_client.h"
#include "server/matchmaking/match_created_handler.h"
#include "server/matchmaking/matchmaking_service.h"
#include "server/player.h"
#include "server/player_session.h"
#include "server/room/room_manager.h"
#include "server/websocket_server.h"
#include "test_helpers.h"

#include <doctest/doctest.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr const char* kServiceToken = "integration-test-token";

void accept_until_count(kfc::WebSocketServer& server, const std::size_t expected_count) {
    for (int attempt = 0; attempt < 1000 && server.clients().size() < expected_count; ++attempt) {
        server.try_accept();
        if (server.clients().size() < expected_count) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

struct SessionFixture {
    kfc::WebSocketServer server{0};
    std::vector<std::unique_ptr<kfc::WebSocketClient>> clients;
    std::size_t next_session_id_ = 0;

    kfc::PlayerSession& make_session(const kfc::UserId user_id, const std::string& username) {
        const std::size_t expected_count = server.clients().size() + 1;
        clients.push_back(std::make_unique<kfc::WebSocketClient>("127.0.0.1", server.port()));
        std::thread accept_thread{[&]() { accept_until_count(server, expected_count); }};
        clients.back()->connect();
        accept_thread.join();
        REQUIRE_EQ(server.clients().size(), expected_count);

        sessions_.push_back(std::make_unique<kfc::PlayerSession>(next_session_id_++,
                                                                   &server.clients().back()));
        sessions_.back()->assign_user(user_id, username, 1000);
        sessions_.back()->bind_player(kfc::Player{static_cast<int>(user_id), username, 1000});
        return *sessions_.back();
    }

private:
    std::vector<std::unique_ptr<kfc::PlayerSession>> sessions_;
};

class StubGameRepository : public kfc::IGameRepository {
public:
    [[nodiscard]] std::optional<int> create_game(int, int) override { return 101; }
    [[nodiscard]] bool finish_game(int, int) override { return true; }
    [[nodiscard]] bool finish_game_without_winner(int) override { return true; }
};

class RecordingGameGateway : public kfc::IGameGateway {
public:
    void notify_match_found(kfc::PlayerId, kfc::PlayerId) override {}
    void notify_game_start(kfc::PlayerId, kfc::PlayerId) override {}
    void notify_search_timeout(kfc::PlayerId) override {}

    void send_game_redirect(kfc::PlayerId player,
                            kfc::GatewayGameRedirectInfo redirect_info) override {
        redirects_.push_back({player, std::move(redirect_info)});
    }

    struct RedirectCall {
        kfc::PlayerId player;
        kfc::GatewayGameRedirectInfo info;
    };

    [[nodiscard]] const std::vector<RedirectCall>& redirects() const noexcept {
        return redirects_;
    }

private:
    std::vector<RedirectCall> redirects_;
};

struct AllocationHarness {
    kfc::InMemoryRuntimeStore runtime_store;
    kfc::BoardModel board;
    kfc::RoomManager room_manager;
    kfc::LocalGameHost local_game_host;
    kfc::GameAllocationHandler allocation_handler;
    kfc::GameCreationListener creation_listener;
    kfc::RuntimeStoreGameServerRegistry registry;
    kfc::RemoteGameAllocator allocator;
    StubGameRepository repository;
    kfc::MatchLifecycleHandler lifecycle_handler;
    kfc::app::MatchmakingConfig matchmaking_config;
    kfc::MatchmakingService matchmaking_service;
    RecordingGameGateway gateway;

    explicit AllocationHarness(const std::string& game_endpoint)
        : board(kfc::test::make_board({{"wK", ".", "bK"}, {".", ".", "."}})),
          room_manager(board),
          local_game_host(room_manager, "game-integration-1", game_endpoint),
          allocation_handler(local_game_host, runtime_store, "game-integration-1", game_endpoint),
          creation_listener("127.0.0.1", 0, allocation_handler, kServiceToken),
          registry(runtime_store, std::chrono::seconds(10)),
          allocator(registry, kServiceToken, std::chrono::milliseconds(1000), 3),
          lifecycle_handler(allocator, repository, runtime_store, "gateway-integration"),
          matchmaking_service(lifecycle_handler, matchmaking_config) {
        kfc::app::ServerMetrics metrics;
        metrics.endpoint = game_endpoint;
        metrics.allocation_endpoint =
            "http://127.0.0.1:" + std::to_string(creation_listener.port()) + "/allocate";
        metrics.active_rooms = 0;
        runtime_store.publish_server_heartbeat("game-integration-1", "local", metrics);

        lifecycle_handler.bind_matchmaking_service(matchmaking_service);
        lifecycle_handler.bind_game_gateway(gateway);
    }

    ~AllocationHarness() { creation_listener.stop(); }
};

}  // namespace

TEST_CASE("AllocationIntegrationTest - GatewayAllocatesRoomThroughGameServer") {
    const std::string game_endpoint = "ws://127.0.0.1:9876";
    AllocationHarness harness(game_endpoint);
    SessionFixture fixture;

    kfc::PlayerSession& white = fixture.make_session(1, "integration_white");
    kfc::PlayerSession& black = fixture.make_session(2, "integration_black");
    white.request_play();
    black.request_play();

    const auto now = std::chrono::steady_clock::now();
    CHECK_FALSE(harness.matchmaking_service.enqueue(white, now).has_value());
    const std::optional<kfc::MatchCreated> match =
        harness.matchmaking_service.enqueue(black, now);
    REQUIRE(match.has_value());
    harness.lifecycle_handler.notify_match_created(*match);

    CHECK_GT(match->room_id, 0u);
    CHECK_EQ(harness.room_manager.active_room_count(), 1u);

    const std::optional<kfc::GameServerLocation> white_location =
        harness.runtime_store.find_player_location(1);
    const std::optional<kfc::GameServerLocation> black_location =
        harness.runtime_store.find_player_location(2);
    REQUIRE(white_location.has_value());
    REQUIRE(black_location.has_value());
    CHECK_EQ(white_location->endpoint, game_endpoint);
    CHECK_EQ(black_location->endpoint, game_endpoint);
    CHECK_EQ(white_location->room_id, match->room_id);
    CHECK_EQ(black_location->room_id, match->room_id);

    REQUIRE_EQ(harness.gateway.redirects().size(), 2u);
    CHECK_EQ(harness.gateway.redirects()[0].info.endpoint, game_endpoint);
    CHECK_EQ(harness.gateway.redirects()[1].info.endpoint, game_endpoint);
    CHECK_EQ(harness.gateway.redirects()[0].info.room_id, match->room_id);
    CHECK_EQ(harness.gateway.redirects()[1].info.room_id, match->room_id);
}

TEST_CASE("AllocationIntegrationTest - RejectsMissingServiceToken") {
    const std::string game_endpoint = "ws://127.0.0.1:9877";
    AllocationHarness harness(game_endpoint);

    const kfc::GameCreationRequest request{1, 2, std::nullopt};
    kfc::RemoteGameAllocator bad_token_allocator(harness.registry, "wrong-token",
                                                 std::chrono::milliseconds(1000), 1);
    CHECK_THROWS_AS(bad_token_allocator.allocate_game(request), std::runtime_error);
}

TEST_CASE("AllocationIntegrationTest - RegistryDiscoversRegisteredGameServer") {
    const std::string game_endpoint = "ws://127.0.0.1:9878";
    AllocationHarness harness(game_endpoint);

    const std::vector<kfc::GameServerRecord> servers = harness.registry.list_available_servers();
    REQUIRE_EQ(servers.size(), 1u);
    CHECK_EQ(servers[0].server_id, "game-integration-1");
    CHECK_EQ(servers[0].endpoint, game_endpoint);
    CHECK_FALSE(servers[0].allocation_endpoint.empty());
}
