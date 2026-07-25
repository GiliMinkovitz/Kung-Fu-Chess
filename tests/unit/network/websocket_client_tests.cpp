#include "network/websocket_client.h"
#include "server/websocket_server.h"
#include "test/socket_test_hooks.h"

#include <doctest/doctest.h>

#include <boost/asio/connect.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/websocket/error.hpp>
#include <boost/system/system_error.hpp>
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

void connect_with_server_accept(kfc::WebSocketServer& server, kfc::WebSocketClient& client) {
    const std::size_t expected_count = server.clients().size() + 1;
    std::thread accept_thread{[&]() {
        for (int attempt = 0; attempt < 1000 && server.clients().size() < expected_count;
             ++attempt) {
            server.try_accept();
            if (server.clients().size() < expected_count) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }};
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

TEST_CASE("WebSocketClientTest - ConnectIsIdempotent") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};

    connect_with_server_accept(server, client);
    client.connect();
    CHECK(client.is_connected());
}

TEST_CASE("WebSocketClientTest - DisconnectWithoutConnectIsSafe") {
    kfc::WebSocketClient client{"127.0.0.1", 19876};
    client.disconnect();
    CHECK_FALSE(client.is_connected());
}

TEST_CASE("WebSocketClientTest - ConnectToClosedPortFails") {
    kfc::WebSocketClient client{"127.0.0.1", 19876};
    CHECK_THROWS_AS(client.connect(), boost::system::system_error);
    CHECK_FALSE(client.is_connected());
}

TEST_CASE("WebSocketClientTest - HandlesAbruptServerCloseOnSendAndReceive") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};

    connect_with_server_accept(server, client);
    REQUIRE_EQ(server.clients().size(), 1u);

    server.clients().front().close();
    server.clients().clear();
    server.prune_disconnected();

    for (int attempt = 0; attempt < 100 && client.is_connected(); ++attempt) {
        (void)client.try_send("select 0 0");
        (void)client.try_receive_snapshot();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    CHECK_FALSE(client.try_receive_snapshot().has_value());
    CHECK_FALSE(client.is_connected());
}

TEST_CASE("WebSocketClientTest - HandshakeFailureLeavesClientDisconnected") {
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::acceptor acceptor{
        io_context, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 0}};
    const auto port = acceptor.local_endpoint().port();

    std::thread accept_thread{[&]() {
        boost::asio::ip::tcp::socket socket{io_context};
        boost::system::error_code accept_ec;
        acceptor.accept(socket, accept_ec);
        if (!accept_ec) {
            const char response[] = "HTTP/1.1 200 OK\r\n\r\n";
            boost::asio::write(socket, boost::asio::buffer(response), accept_ec);
        }
    }};

    kfc::WebSocketClient client{"127.0.0.1", port};
    CHECK_THROWS_AS(client.connect(), boost::system::system_error);
    CHECK_FALSE(client.is_connected());
    accept_thread.join();
}

TEST_CASE("WebSocketClientHookTest - HandlesInjectedWriteWouldBlock") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};
    connect_with_server_accept(server, client);

    kfc::test::SocketTestHooks::next_write_error = boost::asio::error::would_block;
    CHECK_FALSE(client.try_send("blocked"));
    kfc::test::SocketTestHooks::reset();
}

TEST_CASE("WebSocketClientHookTest - HandlesInjectedWriteClosed") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};
    connect_with_server_accept(server, client);

    kfc::test::SocketTestHooks::next_write_error = boost::beast::websocket::error::closed;
    CHECK_FALSE(client.try_send("closed"));
    CHECK_FALSE(client.is_connected());
    kfc::test::SocketTestHooks::reset();
}

TEST_CASE("WebSocketClientHookTest - HandlesInjectedWriteFailure") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};
    connect_with_server_accept(server, client);

    kfc::test::SocketTestHooks::next_write_error =
        boost::system::error_code{6, boost::system::system_category()};
    CHECK_THROWS_AS(client.try_send("fail"), boost::system::system_error);
    CHECK_FALSE(client.is_connected());
    kfc::test::SocketTestHooks::reset();
}

TEST_CASE("WebSocketClientHookTest - HandlesInjectedAvailError") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};
    connect_with_server_accept(server, client);

    kfc::test::SocketTestHooks::next_avail_error =
        boost::system::error_code{7, boost::system::system_category()};
    CHECK_FALSE(client.try_receive_snapshot().has_value());
    CHECK_FALSE(client.is_connected());
    kfc::test::SocketTestHooks::reset();
}

TEST_CASE("WebSocketClientHookTest - HandlesInjectedNonBlockingError") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};
    connect_with_server_accept(server, client);

    kfc::test::SocketTestHooks::force_non_blocking_error = true;
    CHECK_FALSE(client.try_receive_snapshot().has_value());
    CHECK_FALSE(client.is_connected());
    kfc::test::SocketTestHooks::reset();
}

TEST_CASE("WebSocketClientHookTest - HandlesInjectedPeekError") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};
    connect_with_server_accept(server, client);

    kfc::test::SocketTestHooks::next_peek_error = boost::asio::error::connection_aborted;
    CHECK_FALSE(client.try_receive_snapshot().has_value());
    CHECK_FALSE(client.is_connected());
    kfc::test::SocketTestHooks::reset();
}

TEST_CASE("WebSocketClientHookTest - HandlesInjectedReadFailure") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};
    connect_with_server_accept(server, client);
    REQUIRE(server.clients().front().try_send("snapshot"));

    kfc::test::SocketTestHooks::next_read_error =
        boost::system::error_code{8, boost::system::system_category()};
    CHECK_THROWS_AS(client.try_receive_snapshot(), boost::system::system_error);
    CHECK_FALSE(client.is_connected());
    kfc::test::SocketTestHooks::reset();
}

TEST_CASE("WebSocketClientHookTest - HandlesGenericPeekErrorWithPendingData") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};
    connect_with_server_accept(server, client);

    kfc::test::SocketTestHooks::next_peek_error =
        boost::system::error_code{10, boost::system::system_category()};
    kfc::test::SocketTestHooks::forced_peeked = 1;
    CHECK_FALSE(client.try_receive_snapshot().has_value());
    CHECK_FALSE(client.is_connected());
    kfc::test::SocketTestHooks::reset();
}

TEST_CASE("WebSocketClientHookTest - HandlesInjectedReadClosedError") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};
    connect_with_server_accept(server, client);
    REQUIRE(server.clients().front().try_send("snapshot"));

    kfc::test::SocketTestHooks::next_read_error = boost::beast::websocket::error::closed;
    CHECK_FALSE(client.try_receive_snapshot().has_value());
    CHECK_FALSE(client.is_connected());
    kfc::test::SocketTestHooks::reset();
}
