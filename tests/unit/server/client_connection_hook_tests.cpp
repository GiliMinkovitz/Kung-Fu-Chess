#include "network/websocket_client.h"
#include "server/client_connection.h"
#include "server/websocket_server.h"
#include "test/socket_test_hooks.h"

#include <doctest/doctest.h>

#include <boost/asio/error.hpp>
#include <boost/beast/websocket/error.hpp>
#include <boost/system/system_error.hpp>
#include <chrono>
#include <thread>

namespace {

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
    REQUIRE_EQ(server.clients().size(), expected_count);
}

}  // namespace

TEST_CASE("ClientConnectionHookTest - HandlesInjectedAvailErrorOnRead") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};
    connect_with_server_accept(server, client);

    kfc::test::SocketTestHooks::next_avail_error =
        boost::system::error_code{1, boost::system::system_category()};
    CHECK_FALSE(server.clients().front().try_read().has_value());
    CHECK_FALSE(server.clients().front().is_open());
    kfc::test::SocketTestHooks::reset();
}

TEST_CASE("ClientConnectionHookTest - HandlesInjectedReadWouldBlock") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};
    connect_with_server_accept(server, client);
    REQUIRE(client.try_send("payload"));

    kfc::test::SocketTestHooks::next_read_error = boost::asio::error::would_block;
    CHECK_FALSE(server.clients().front().try_read().has_value());
    kfc::test::SocketTestHooks::reset();
}

TEST_CASE("ClientConnectionHookTest - HandlesInjectedReadClosed") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};
    connect_with_server_accept(server, client);
    REQUIRE(client.try_send("payload"));

    kfc::test::SocketTestHooks::next_read_error = boost::beast::websocket::error::closed;
    CHECK_FALSE(server.clients().front().try_read().has_value());
    CHECK_FALSE(server.clients().front().is_open());
    kfc::test::SocketTestHooks::reset();
}

TEST_CASE("ClientConnectionHookTest - HandlesInjectedWriteWouldBlock") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};
    connect_with_server_accept(server, client);

    kfc::test::SocketTestHooks::next_write_error = boost::asio::error::would_block;
    CHECK_FALSE(server.clients().front().try_send("blocked"));
    kfc::test::SocketTestHooks::reset();
}

TEST_CASE("ClientConnectionHookTest - HandlesInjectedWriteClosed") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};
    connect_with_server_accept(server, client);

    kfc::test::SocketTestHooks::next_write_error = boost::beast::websocket::error::closed;
    CHECK_FALSE(server.clients().front().try_send("closed"));
    CHECK_FALSE(server.clients().front().is_open());
    kfc::test::SocketTestHooks::reset();
}

TEST_CASE("ClientConnectionHookTest - HandlesInjectedNonBlockingError") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};
    connect_with_server_accept(server, client);

    kfc::test::SocketTestHooks::force_non_blocking_error = true;
    server.clients().front().probe_disconnect();
    CHECK_FALSE(server.clients().front().is_open());
    kfc::test::SocketTestHooks::reset();
}

TEST_CASE("ClientConnectionHookTest - HandlesInjectedPeekError") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};
    connect_with_server_accept(server, client);

    kfc::test::SocketTestHooks::next_peek_error = boost::asio::error::connection_aborted;
    server.clients().front().probe_disconnect();
    CHECK_FALSE(server.clients().front().is_open());
    kfc::test::SocketTestHooks::reset();
}

TEST_CASE("ClientConnectionHookTest - HandlesInjectedAvailErrorOnProbe") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};
    connect_with_server_accept(server, client);

    kfc::test::SocketTestHooks::next_avail_error =
        boost::system::error_code{3, boost::system::system_category()};
    server.clients().front().probe_disconnect();
    CHECK_FALSE(server.clients().front().is_open());
    kfc::test::SocketTestHooks::reset();
}

TEST_CASE("ClientConnectionHookTest - HandlesInjectedGenericReadError") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};
    connect_with_server_accept(server, client);
    REQUIRE(client.try_send("payload"));

    kfc::test::SocketTestHooks::next_read_error =
        boost::system::error_code{4, boost::system::system_category()};
    CHECK_FALSE(server.clients().front().try_read().has_value());
    CHECK_FALSE(server.clients().front().is_open());
    kfc::test::SocketTestHooks::reset();
}

TEST_CASE("ClientConnectionHookTest - HandlesInjectedGenericWriteError") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};
    connect_with_server_accept(server, client);

    kfc::test::SocketTestHooks::next_write_error =
        boost::system::error_code{5, boost::system::system_category()};
    CHECK_FALSE(server.clients().front().try_send("fail"));
    CHECK_FALSE(server.clients().front().is_open());
    kfc::test::SocketTestHooks::reset();
}

TEST_CASE("ClientConnectionHookTest - HandlesGenericPeekErrorWithPendingData") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};
    connect_with_server_accept(server, client);

    kfc::test::SocketTestHooks::next_peek_error =
        boost::system::error_code{9, boost::system::system_category()};
    kfc::test::SocketTestHooks::forced_peeked = 1;
    server.clients().front().probe_disconnect();
    CHECK_FALSE(server.clients().front().is_open());
    kfc::test::SocketTestHooks::reset();
}

TEST_CASE("ClientConnectionHookTest - TreatsSuccessfulPeekWithDataAsConnected") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};
    connect_with_server_accept(server, client);

    kfc::test::SocketTestHooks::force_peek_success_with_data = true;
    kfc::test::SocketTestHooks::forced_peeked = 1;
    server.clients().front().probe_disconnect();
    CHECK(server.clients().front().is_open());
    kfc::test::SocketTestHooks::reset();
}
