#include "network/network_login_session.h"
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

struct ConnectedSession {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client;
    kfc::NetworkInputHandler handler;
    kfc::NetworkLoginSession login_session;

    ConnectedSession()
        : client{"127.0.0.1", server.port()},
          handler{client},
          login_session{handler} {
        connect_with_server_accept(server, client);
    }

    ~ConnectedSession() {
        client.disconnect();
        server.clients().clear();
        server.prune_disconnected();
    }
};

}  // namespace

TEST_CASE("NetworkLoginSessionTest - ResolveUsernameUsesInputWhenProvided") {
    CHECK_EQ(kfc::NetworkLoginSession::resolve_username("alice"), "alice");
}

TEST_CASE("NetworkLoginSessionTest - ResolveUsernameGeneratesUniqueDefault") {
    const std::string first = kfc::NetworkLoginSession::resolve_username("");
    const std::string second = kfc::NetworkLoginSession::resolve_username("");
    CHECK_FALSE(first.empty());
    CHECK(first.rfind("Player", 0) == 0);
    CHECK_EQ(first, second);
}

TEST_CASE("NetworkLoginSessionTest - SendsLoginAndWaitsForLoginOkBeforePlay") {
    ConnectedSession session;
    REQUIRE(session.login_session.send_login("alice"));
    CHECK(session.login_session.phase() == kfc::NetworkLoginPhase::LoginSent);

    const auto login_message = poll_server_message(session.server);
    REQUIRE(login_message.has_value());
    CHECK_EQ(*login_message, "login alice alice");

    session.login_session.handle_message("login_ok 1200");
    CHECK(session.login_session.phase() == kfc::NetworkLoginPhase::LoginOk);
    CHECK_EQ(session.login_session.rating(), 1200);

    REQUIRE(session.login_session.try_send_play());
    CHECK(session.login_session.phase() == kfc::NetworkLoginPhase::PlayRequested);
    CHECK(session.login_session.is_play_requested());

    const auto play_message = poll_server_message(session.server);
    REQUIRE(play_message.has_value());
    CHECK_EQ(*play_message, "play");
}

TEST_CASE("NetworkLoginSessionTest - LoginFailedStopsBeforePlay") {
    ConnectedSession session;
    REQUIRE(session.login_session.send_login("alice"));

    session.login_session.handle_message("login_failed already_connected");
    CHECK(session.login_session.is_login_failed());
    CHECK_EQ(session.login_session.login_failure_reason(), "already_connected");
    CHECK_FALSE(session.login_session.try_send_play());
    CHECK(session.login_session.phase() == kfc::NetworkLoginPhase::LoginFailed);
}

TEST_CASE("NetworkLoginSessionTest - IgnoresUnrelatedMessagesWhileWaitingForLogin") {
    ConnectedSession session;
    REQUIRE(session.login_session.send_login("bob"));

    session.login_session.handle_message("searching");
    CHECK(session.login_session.phase() == kfc::NetworkLoginPhase::LoginSent);
}

TEST_CASE("NetworkLoginSessionTest - ResetAllowsRetryAfterFailure") {
    ConnectedSession session;
    REQUIRE(session.login_session.send_login("alice"));

    const auto first_login = poll_server_message(session.server);
    REQUIRE(first_login.has_value());
    CHECK_EQ(*first_login, "login alice alice");

    session.login_session.handle_message("login_failed already_connected");
    CHECK(session.login_session.is_login_failed());

    session.login_session.reset();
    CHECK(session.login_session.phase() == kfc::NetworkLoginPhase::Connected);
    REQUIRE(session.login_session.send_login("bob"));

    const auto login_message = poll_server_message(session.server);
    REQUIRE(login_message.has_value());
    CHECK_EQ(*login_message, "login bob bob");
}
