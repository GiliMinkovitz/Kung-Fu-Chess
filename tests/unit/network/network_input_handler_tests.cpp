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

void connect_with_server_accept(kfc::WebSocketServer& server,
                                kfc::WebSocketClient& client) {
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

struct ConnectedSession {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client;
    kfc::NetworkInputHandler handler;

    ConnectedSession()
        : client{"127.0.0.1", server.port()}, handler{client} {
        connect_with_server_accept(server, client);
    }

    ~ConnectedSession() {
        client.disconnect();
        server.clients().clear();
        server.prune_disconnected();
    }
};

}  // namespace

TEST_CASE("NetworkInputHandlerTest - SendsSelectCommand") {
    ConnectedSession session;
    REQUIRE(session.handler.send_select(1, 2));

    const auto message = poll_server_message(session.server);
    REQUIRE(message.has_value());
    CHECK_EQ(*message, "select 1 2");
}

TEST_CASE("NetworkInputHandlerTest - SendsMoveCommand") {
    ConnectedSession session;
    REQUIRE(session.handler.send_move(3, 4));

    const auto message = poll_server_message(session.server);
    REQUIRE(message.has_value());
    CHECK_EQ(*message, "move 3 4");
}

TEST_CASE("NetworkInputHandlerTest - SendsJumpCommand") {
    ConnectedSession session;
    REQUIRE(session.handler.send_jump(5, 6));

    const auto message = poll_server_message(session.server);
    REQUIRE(message.has_value());
    CHECK_EQ(*message, "jump 5 6");
}

TEST_CASE("NetworkInputHandlerTest - SendsClearCommand") {
    ConnectedSession session;
    REQUIRE(session.handler.send_clear());

    const auto message = poll_server_message(session.server);
    REQUIRE(message.has_value());
    CHECK_EQ(*message, "clear");
}

TEST_CASE("NetworkInputHandlerTest - ReturnsFalseWhenDisconnected") {
    kfc::WebSocketClient client{"127.0.0.1", 19877};
    kfc::NetworkInputHandler handler{client};

    CHECK_FALSE(handler.send_select(0, 0));
    CHECK_FALSE(handler.send_move(0, 1));
    CHECK_FALSE(handler.send_jump(0, 0));
    CHECK_FALSE(handler.send_clear());
}
