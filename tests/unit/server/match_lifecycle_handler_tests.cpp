#include "app/in_memory_runtime_store.h"
#include "database/i_game_repository.h"
#include "model/piece.h"
#include "network/websocket_client.h"
#include "server/game/i_game_allocator.h"
#include "server/game/protocol/game_creation_request.h"
#include "server/game/protocol/game_creation_response.h"
#include "server/gateway/game_redirect_info.h"
#include "server/gateway/i_game_gateway.h"
#include "server/match/match_lifecycle_handler.h"
#include "server/player.h"
#include "server/player_session.h"
#include "server/websocket_server.h"

#include <doctest/doctest.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace {

void accept_until_count(kfc::WebSocketServer& server, std::size_t expected_count) {
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
    [[nodiscard]] std::optional<int> create_game(int, int) override { return 99; }
    [[nodiscard]] bool finish_game(int, int) override { return true; }
    [[nodiscard]] bool finish_game_without_winner(int) override { return true; }
};

class StubGameAllocator : public kfc::IGameAllocator {
public:
    explicit StubGameAllocator(kfc::GameCreationResponse response)
        : response_(std::move(response)) {}

    kfc::GameCreationResponse allocate_game(const kfc::GameCreationRequest&) override {
        return response_;
    }

private:
    kfc::GameCreationResponse response_;
};

class RecordingGameGateway : public kfc::IGameGateway {
public:
    void notify_match_found(kfc::PlayerId, kfc::PlayerId) override {}
    void notify_game_start(kfc::PlayerId, kfc::PlayerId) override {}
    void notify_search_timeout(kfc::PlayerId) override {}

    void send_game_redirect(kfc::PlayerId player,
                            kfc::GameRedirectInfo redirect_info) override {
        redirects_.push_back({player, std::move(redirect_info)});
    }

    struct RedirectCall {
        kfc::PlayerId player;
        kfc::GameRedirectInfo info;
    };

    [[nodiscard]] const std::vector<RedirectCall>& redirects() const noexcept {
        return redirects_;
    }

private:
    std::vector<RedirectCall> redirects_;
};

}  // namespace

TEST_CASE("MatchLifecycleHandlerTest - SendsRedirectAfterAllocation") {
    SessionFixture fixture;
    kfc::PlayerSession& white = fixture.make_session(1, "white_player");
    kfc::PlayerSession& black = fixture.make_session(2, "black_player");

    const kfc::RoomId allocated_room_id = 55;
    const kfc::GameCreationResponse allocation_response{
        allocated_room_id, "game-server-1", std::nullopt};

    StubGameRepository repository;
    StubGameAllocator allocator{allocation_response};
    kfc::InMemoryRuntimeStore runtime_store{"ws://localhost:8765"};
    RecordingGameGateway gateway;

    kfc::MatchLifecycleHandler handler{allocator, repository, runtime_store, "gateway-server"};
    handler.bind_game_gateway(gateway);

    const kfc::RoomId room_id = handler.create_match(&white, &black);

    CHECK_EQ(room_id, allocated_room_id);
    REQUIRE_EQ(gateway.redirects().size(), 2u);

    CHECK_EQ(gateway.redirects()[0].player, 0u);
    CHECK_EQ(gateway.redirects()[0].info.room_id, allocated_room_id);
    CHECK_EQ(gateway.redirects()[0].info.server_id, "game-server-1");
    CHECK_EQ(gateway.redirects()[0].info.endpoint, "ws://localhost:8765");
    CHECK_EQ(gateway.redirects()[0].info.side, kfc::PieceColor::White);

    CHECK_EQ(gateway.redirects()[1].player, 1u);
    CHECK_EQ(gateway.redirects()[1].info.room_id, allocated_room_id);
    CHECK_EQ(gateway.redirects()[1].info.server_id, "game-server-1");
    CHECK_EQ(gateway.redirects()[1].info.endpoint, "ws://localhost:8765");
    CHECK_EQ(gateway.redirects()[1].info.side, kfc::PieceColor::Black);
}

TEST_CASE("MatchLifecycleHandlerTest - PrefersAllocationEndpointWhenProvided") {
    SessionFixture fixture;
    kfc::PlayerSession& white = fixture.make_session(10, "endpoint_white");
    kfc::PlayerSession& black = fixture.make_session(11, "endpoint_black");

    const kfc::GameCreationResponse allocation_response{
        88, "remote-server", std::string{"ws://remote.example:7777"}};

    StubGameRepository repository;
    StubGameAllocator allocator{allocation_response};
    kfc::InMemoryRuntimeStore runtime_store{"ws://localhost:8765"};
    RecordingGameGateway gateway;

    kfc::MatchLifecycleHandler handler{allocator, repository, runtime_store, "gateway-server"};
    handler.bind_game_gateway(gateway);

    handler.create_match(&white, &black);

    REQUIRE_EQ(gateway.redirects().size(), 2u);
    CHECK_EQ(gateway.redirects()[0].info.endpoint, "ws://remote.example:7777");
    CHECK_EQ(gateway.redirects()[1].info.endpoint, "ws://remote.example:7777");
}

TEST_CASE("MatchLifecycleHandlerTest - SkipsRedirectWhenGatewayUnbound") {
    SessionFixture fixture;
    kfc::PlayerSession& white = fixture.make_session(20, "no_gateway_white");
    kfc::PlayerSession& black = fixture.make_session(21, "no_gateway_black");

    StubGameRepository repository;
    StubGameAllocator allocator{kfc::GameCreationResponse{1, "server", std::nullopt}};
    kfc::InMemoryRuntimeStore runtime_store{"ws://localhost:8765"};

    kfc::MatchLifecycleHandler handler{allocator, repository, runtime_store, "gateway-server"};

    CHECK_NOTHROW(handler.create_match(&white, &black));
}
