#include "network/websocket_client.h"
#include "server/websocket_server.h"

#include <doctest/doctest.h>

#include <chrono>
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

void connect_with_server_accept(kfc::WebSocketServer& server,
                                kfc::WebSocketClient& client) {
    std::thread accept_thread{[&]() { accept_one_client(server); }};
    client.connect();
    accept_thread.join();
}

std::optional<std::string> poll_client_snapshot(kfc::WebSocketClient& client) {
    for (int attempt = 0; attempt < 100; ++attempt) {
        if (const auto snapshot = client.try_receive_snapshot()) {
            return snapshot;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return std::nullopt;
}

}  // namespace

TEST_CASE("WebSocketClientTest - StartsDisconnected") {
    kfc::WebSocketClient client{"127.0.0.1", 19876};

    CHECK_FALSE(client.is_connected());
    CHECK_FALSE(client.try_send("select 0 0"));
    CHECK_FALSE(client.try_receive_snapshot().has_value());
}

TEST_CASE("WebSocketClientTest - ConnectSendReceiveAndDisconnect") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};

    connect_with_server_accept(server, client);
    CHECK(client.is_connected());

    REQUIRE_EQ(server.clients().size(), 1u);

    const std::string snapshot =
        "snapshot\nclock_ms 42\ngame_over false\nheight 1\nwidth 1\ncells\nwK";
    REQUIRE(server.clients().front().try_send(snapshot));

    const auto received = poll_client_snapshot(client);
    REQUIRE(received.has_value());
    CHECK_EQ(*received, snapshot);

    CHECK(client.try_send("select 0 0"));

    client.disconnect();
    CHECK_FALSE(client.is_connected());
    CHECK_FALSE(client.try_send("move 0 1"));
}

TEST_CASE("WebSocketClientTest - DetectsServerDisconnect") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};

    connect_with_server_accept(server, client);
    REQUIRE_EQ(server.clients().size(), 1u);

    server.clients().clear();
    server.prune_disconnected();

    for (int attempt = 0; attempt < 100 && client.is_connected(); ++attempt) {
        (void)client.try_receive_snapshot();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    CHECK_FALSE(client.is_connected());
}
