#include "app/server_config.h"
#include "network/websocket_client.h"
#include "server/websocket_server.h"

#include <doctest/doctest.h>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/system_error.hpp>
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

void connect_client(kfc::WebSocketServer& server, kfc::WebSocketClient& client) {
    const std::size_t expected_count = server.clients().size() + 1;
    std::thread accept_thread{[&]() { accept_until_count(server, expected_count); }};
    client.connect();
    accept_thread.join();
}

std::optional<std::string> poll_message(kfc::WebSocketClient& client) {
    for (int attempt = 0; attempt < 100; ++attempt) {
        if (const auto message = client.try_receive_snapshot()) {
            return message;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return std::nullopt;
}

}  // namespace

TEST_CASE("WebSocketServerTest - BroadcastsToConnectedClients") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};

    connect_client(server, client);
    REQUIRE_EQ(server.clients().size(), 1u);

    server.broadcast("match_found white");
    const auto received = poll_message(client);
    REQUIRE(received.has_value());
    CHECK(*received == "match_found white");
}

TEST_CASE("WebSocketServerTest - EnforcesMaxClients") {
    kfc::WebSocketServer server{0};
    const std::size_t max_clients = kfc::app::ServerConfig::kDefaultMaxClients;
    std::vector<std::unique_ptr<kfc::WebSocketClient>> clients;
    clients.reserve(max_clients);

    for (std::size_t i = 0; i < max_clients; ++i) {
        clients.push_back(
            std::make_unique<kfc::WebSocketClient>("127.0.0.1", server.port()));
        connect_client(server, *clients.back());
    }
    CHECK_EQ(server.clients().size(), max_clients);

    server.try_accept();
    CHECK_EQ(server.clients().size(), max_clients);
}

TEST_CASE("WebSocketServerTest - PrunesDisconnectedClients") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};

    connect_client(server, client);
    REQUIRE_EQ(server.clients().size(), 1u);

    client.disconnect();
    CHECK_FALSE(server.clients().front().try_send("ping"));
    server.prune_disconnected();
    CHECK(server.clients().empty());
}

TEST_CASE("WebSocketServerTest - RejectsInvalidHandshake") {
    kfc::WebSocketServer server{0};
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::socket socket{io_context};
    boost::system::error_code connect_ec;
    boost::asio::connect(socket,
                         boost::asio::ip::tcp::resolver{io_context}.resolve(
                             "127.0.0.1", std::to_string(server.port())),
                         connect_ec);
    REQUIRE_FALSE(connect_ec);
    socket.write_some(boost::asio::buffer("GET / HTTP/1.1\r\n\r\n"), connect_ec);
    REQUIRE_FALSE(connect_ec);

    CHECK_THROWS_AS(server.try_accept(), boost::system::system_error);
    server.prune_disconnected();
    CHECK(server.clients().empty());
}

TEST_CASE("WebSocketServerTest - ThrowsWhenAcceptorClosed") {
    kfc::WebSocketServer server{0};
    server.close_acceptor_for_tests();
    CHECK_THROWS_AS(server.try_accept(), boost::system::system_error);
}
