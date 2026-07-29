#include "network/network_game_redirect_flow.h"
#include "network/network_input_handler.h"
#include "network/websocket_client.h"
#include "server/websocket_server.h"

#include <doctest/doctest.h>

#include <chrono>
#include <optional>
#include <string>
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

void connect_with_server_accept(kfc::WebSocketServer& server, kfc::WebSocketClient& client) {
    std::thread accept_thread{[&]() { accept_one_client(server); }};
    client.connect();
    accept_thread.join();
}

std::optional<std::string> poll_server_message(kfc::WebSocketServer& server) {
    for (int attempt = 0; attempt < 100; ++attempt) {
        for (kfc::ClientConnection& connection : server.clients()) {
            if (const auto message = connection.try_read()) {
                return message;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return std::nullopt;
}

}  // namespace

TEST_CASE("NetworkGameRedirectFlowTest - ClosesLobbyOpensGameAndSendsJoinGame") {
    kfc::WebSocketServer lobby_server{0};
    kfc::WebSocketServer game_server{0};

    kfc::WebSocketClient lobby_client{"127.0.0.1", lobby_server.port()};
    kfc::WebSocketClient game_client{"127.0.0.1", game_server.port()};
    connect_with_server_accept(lobby_server, lobby_client);

    kfc::NetworkInputHandler game_input{game_client};
    kfc::NetworkGameRedirectFlow redirect_flow{lobby_client, game_client, game_input};

    const kfc::GameRedirectInfo redirect{
        "ws://127.0.0.1:" + std::to_string(game_server.port()), 42, kfc::PieceColor::White};

    std::thread accept_thread{[&]() { accept_one_client(game_server); }};
    REQUIRE(redirect_flow.execute(redirect));
    accept_thread.join();

    CHECK(redirect_flow.lobby_closed());
    CHECK_FALSE(lobby_client.is_connected());
    CHECK(redirect_flow.game_connected());
    CHECK(game_client.is_connected());
    CHECK(redirect_flow.join_game_sent());

    const auto join_message = poll_server_message(game_server);
    REQUIRE(join_message.has_value());
    CHECK_EQ(*join_message, "join_game 42");

    game_client.disconnect();
    lobby_server.clients().clear();
    game_server.clients().clear();
    lobby_server.prune_disconnected();
    game_server.prune_disconnected();
}
