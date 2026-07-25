#include "network/websocket_client.h"
#include "server/client_connection.h"
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

}  // namespace

TEST_CASE("ClientConnectionTest - CloseIsIdempotent") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};

    connect_with_server_accept(server, client);
    REQUIRE_EQ(server.clients().size(), 1u);

    kfc::ClientConnection& connection = server.clients().front();
    connection.close();
    CHECK_FALSE(connection.is_open());
    connection.close();
    CHECK_FALSE(connection.is_open());
}

TEST_CASE("ClientConnectionTest - ReadAndSendRequireOpenConnection") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};

    connect_with_server_accept(server, client);
    REQUIRE_EQ(server.clients().size(), 1u);

    kfc::ClientConnection& connection = server.clients().front();
    connection.close();

    CHECK_FALSE(connection.try_read().has_value());
    CHECK_FALSE(connection.try_send("ping"));
    CHECK_FALSE(connection.is_open());
}

TEST_CASE("ClientConnectionTest - DetectsClosedClientAfterDisconnect") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};

    connect_with_server_accept(server, client);
    REQUIRE_EQ(server.clients().size(), 1u);

    client.disconnect();
    server.clients().front().probe_disconnect();
    CHECK_FALSE(server.clients().front().is_open());
}

TEST_CASE("ClientConnectionTest - ReadsClientMessages") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};

    connect_with_server_accept(server, client);
    REQUIRE_EQ(server.clients().size(), 1u);

    REQUIRE(client.try_send("login alice"));
    const auto message = server.clients().front().try_read();
    REQUIRE(message.has_value());
    CHECK(*message == "login alice");
}

TEST_CASE("ClientConnectionTest - PendingClientDataDoesNotSignalDisconnect") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};

    connect_with_server_accept(server, client);
    REQUIRE(client.try_send("queued"));
    CHECK(server.clients().front().try_send("pong"));
    CHECK(server.clients().front().is_open());
}

TEST_CASE("ClientConnectionTest - TryReadAfterDisconnectMarksConnectionClosed") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};

    connect_with_server_accept(server, client);
    client.disconnect();
    CHECK_FALSE(server.clients().front().try_read().has_value());
    CHECK_FALSE(server.clients().front().is_open());
}
