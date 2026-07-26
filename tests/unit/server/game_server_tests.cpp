#include "model/game_config.h"
#include "network/login_message_reader.h"
#include "network/matchmaking_message_reader.h"
#include "network/websocket_client.h"
#include "server/authentication_service.h"
#include "server/game_server.h"
#include "server/rating_service.h"
#include "test/socket_test_hooks.h"
#include "test_helpers.h"

#include <doctest/doctest.h>

#include <chrono>
#include <optional>
#include <sqlite3.h>
#include <string>
#include <thread>

namespace {

void connect_through_server(kfc::GameServer& server, kfc::WebSocketClient& client) {
    const std::size_t expected_count = server.websocket_server().clients().size() + 1;
    std::thread connect_thread{[&]() { client.connect(); }};
    for (int attempt = 0; attempt < 1000 && server.websocket_server().clients().size() < expected_count;
         ++attempt) {
        server.tick_once();
        if (server.websocket_server().clients().size() < expected_count) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    connect_thread.join();
    REQUIRE_EQ(server.websocket_server().clients().size(), expected_count);
}

std::optional<std::string> poll_message(kfc::WebSocketClient& client,
                                        int max_attempts = 200) {
    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        if (const auto message = client.try_receive_snapshot()) {
            return message;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return std::nullopt;
}

std::optional<std::string> poll_snapshot(kfc::WebSocketClient& client) {
    for (int attempt = 0; attempt < 200; ++attempt) {
        if (const auto message = client.try_receive_snapshot()) {
            if (message->find("snapshot") != std::string::npos) {
                return message;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return std::nullopt;
}

std::optional<std::string> poll_login_message(kfc::WebSocketClient& client,
                                              int max_attempts = 200) {
    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        if (const auto message = client.try_receive_snapshot()) {
            if (kfc::read_login_message(*message).has_value()) {
                return message;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return std::nullopt;
}

void login_client(kfc::GameServer& server, kfc::WebSocketClient& client,
                  const std::string& username, const std::string& password = "testpass") {
    connect_through_server(server, client);

    REQUIRE(client.try_send("login " + username + " " + password));
    for (int attempt = 0; attempt < 50; ++attempt) {
        server.tick_once();
        if (const auto message = poll_login_message(client)) {
            const auto result = kfc::read_login_message(*message);
            REQUIRE(result.has_value());
            REQUIRE(result->status == kfc::LoginResultStatus::Ok);
            return;
        }
    }
    FAIL("Expected login_ok response");
}

void login_and_queue(kfc::GameServer& server, kfc::WebSocketClient& client,
                     const std::string& username, const std::string& password = "testpass") {
    login_client(server, client, username, password);

    REQUIRE(client.try_send("play"));
    server.tick_once();
}

void start_match(kfc::GameServer& server, kfc::WebSocketClient& white_client,
                 kfc::WebSocketClient& black_client, const std::string& white_name,
                 const std::string& black_name) {
    login_and_queue(server, white_client, white_name);
    login_and_queue(server, black_client, black_name);

    for (int attempt = 0; attempt < 50 && !server.room().active(); ++attempt) {
        server.tick_once();
        (void)poll_message(white_client);
        (void)poll_message(black_client);
    }

    REQUIRE(server.room().active());
    REQUIRE(server.room().db_game_id().has_value());
}

}  // namespace

TEST_CASE("GameServerTest - InitializesInMemoryDatabase") {
    kfc::GameServer server{0, kfc::test::make_board({{"wK", ".", "bK"}}), ":memory:"};

    CHECK(server.database().connection() != nullptr);
    CHECK_FALSE(server.player_repository().find_by_username("missing").has_value());
}

TEST_CASE("GameServerTest - RejectsInvalidDatabasePath") {
    CHECK_THROWS_AS(
        (kfc::GameServer{0, kfc::test::make_board({{"wK", ".", "bK"}}), "/tmp"}),
        std::runtime_error);
}

TEST_CASE("GameServerTest - AcceptsClientAndProcessesLogin") {
    kfc::GameServer server{0, kfc::test::make_board({{"wK", ".", "bK"}}), ":memory:"};
    kfc::WebSocketClient client{"127.0.0.1", server.websocket_server().port()};

    login_client(server, client, "server_user");

    const auto player = server.player_repository().find_by_username("server_user");
    REQUIRE(player.has_value());
    CHECK_EQ(player->rating(), 1000);
}

TEST_CASE("GameServerTest - MatchmakingFlowFindsOpponents") {
    kfc::GameServer server{0, kfc::test::make_board({{"wK", ".", "bK"}}), ":memory:"};
    kfc::WebSocketClient white_client{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient black_client{"127.0.0.1", server.websocket_server().port()};

    login_client(server, white_client, "white_user");
    REQUIRE(white_client.try_send("play"));
    for (int attempt = 0; attempt < 50 && server.matchmaking().waiting_count() == 0; ++attempt) {
        server.tick_once();
    }
    REQUIRE_EQ(server.matchmaking().waiting_count(), 1u);

    login_and_queue(server, black_client, "black_user");
    for (int attempt = 0; attempt < 50 && !server.room().active(); ++attempt) {
        server.tick_once();
        (void)poll_message(white_client);
        (void)poll_message(black_client);
    }

    CHECK(server.room().active());
    REQUIRE(server.room().db_game_id().has_value());
}

TEST_CASE("GameServerTest - ActiveRoomBroadcastsSnapshotsAndProcessesMoves") {
    kfc::GameServer server{0, kfc::test::make_board({{"wK", ".", "bK"}}), ":memory:"};
    kfc::WebSocketClient white_client{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient black_client{"127.0.0.1", server.websocket_server().port()};

    start_match(server, white_client, black_client, "active_white", "active_black");

    for (int attempt = 0; attempt < 50; ++attempt) {
        server.tick_once();
        const auto white_snapshot = poll_snapshot(white_client);
        const auto black_snapshot = poll_snapshot(black_client);
        if (white_snapshot.has_value() && black_snapshot.has_value()) {
            CHECK(white_snapshot->find("snapshot") != std::string::npos);
            CHECK_EQ(*white_snapshot, *black_snapshot);
            break;
        }
    }

    REQUIRE(white_client.try_send("select 0 0"));
    server.tick_once();
    REQUIRE(white_client.try_send("clear"));
    server.tick_once();
    REQUIRE(white_client.try_send("move 0 1"));
    server.tick_once();
    REQUIRE(black_client.try_send("unknown_command"));
    server.tick_once();
}

TEST_CASE("GameServerTest - FinishesGameAndUpdatesRatings") {
    kfc::GameServer server{
        0, kfc::test::make_board({{"wR", ".", "bK"}, {"wK", ".", "."}}), ":memory:"};
    kfc::WebSocketClient white_client{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient black_client{"127.0.0.1", server.websocket_server().port()};

    start_match(server, white_client, black_client, "winner", "loser");

    const auto winner = server.player_repository().find_by_username("winner");
    const auto loser = server.player_repository().find_by_username("loser");
    REQUIRE(winner.has_value());
    REQUIRE(loser.has_value());
    const int winner_before = winner->rating();
    const int loser_before = loser->rating();
    const kfc::RatingChange expected =
        kfc::RatingService{}.calculate(winner_before, loser_before);
    const int game_id = *server.room().db_game_id();

    kfc::Match& match = server.room().match();
    match.submit_action(kfc::Select{0, 0});
    match.submit_action(kfc::MoveSelected{0, 2});

    while (!match.is_game_over()) {
        match.tick(kfc::kMoveDurationMs);
    }
    server.tick_once();

    REQUIRE_FALSE(server.room().active());

    const auto winner_after = server.player_repository().find_by_username("winner");
    const auto loser_after = server.player_repository().find_by_username("loser");
    REQUIRE(winner_after.has_value());
    REQUIRE(loser_after.has_value());
    CHECK_EQ(winner_after->rating(), expected.winner_new_rating);
    CHECK_EQ(loser_after->rating(), expected.loser_new_rating);

    sqlite3* db = server.database().connection();
    sqlite3_stmt* stmt = nullptr;
    REQUIRE(sqlite3_prepare_v2(db,
                               "SELECT status, winner_id FROM games WHERE id = ? LIMIT 1;", -1,
                               &stmt, nullptr) == SQLITE_OK);
    sqlite3_bind_int(stmt, 1, game_id);
    REQUIRE(sqlite3_step(stmt) == SQLITE_ROW);
    CHECK_EQ(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))),
             "finished");
    CHECK_EQ(sqlite3_column_int(stmt, 1), winner_after->id());
    sqlite3_finalize(stmt);
}

TEST_CASE("GameServerTest - ExposesRepositoriesAndMatchmaking") {
    kfc::GameServer server{0, kfc::test::make_board({{"wK", ".", "bK"}}), ":memory:"};

    CHECK(server.database().connection() != nullptr);
    CHECK_EQ(server.matchmaking().waiting_count(), 0u);
    CHECK(&server.game_repository() == &server.game_repository());
}

TEST_CASE("GameServerTest - RejectsMalformedLoginAndPlayMessages") {
    kfc::GameServer server{0, kfc::test::make_board({{"wK", ".", "bK"}}), ":memory:"};
    kfc::WebSocketClient client{"127.0.0.1", server.websocket_server().port()};

    connect_through_server(server, client);

    REQUIRE(client.try_send("   "));
    server.tick_once();
    CHECK_FALSE(server.player_repository().find_by_username("anyone").has_value());

    REQUIRE(client.try_send("login"));
    server.tick_once();
    CHECK_FALSE(server.player_repository().find_by_username("anyone").has_value());

    REQUIRE(client.try_send("login malformed_alice extra token"));
    server.tick_once();
    CHECK_FALSE(server.player_repository().find_by_username("malformed_alice").has_value());

    REQUIRE(client.try_send("signin nobody"));
    server.tick_once();
    CHECK_FALSE(server.player_repository().find_by_username("nobody").has_value());

    kfc::WebSocketClient valid_client{"127.0.0.1", server.websocket_server().port()};
    login_client(server, valid_client, "valid_play_user");
    REQUIRE(server.player_repository().find_by_username("valid_play_user").has_value());

    REQUIRE(valid_client.try_send("play extra"));
    server.tick_once();
    CHECK_FALSE(server.room().active());

    REQUIRE(valid_client.try_send("play"));
    server.tick_once();
    CHECK_FALSE(server.room().active());
}

TEST_CASE("GameServerTest - IgnoresPlayMessagesWhileSearching") {
    kfc::test::SocketTestHooks::reset();
    kfc::GameServer server{0, kfc::test::make_board({{"wK", ".", "bK"}}), ":memory:"};
    kfc::WebSocketClient client{"127.0.0.1", server.websocket_server().port()};

    login_client(server, client, "queue_user");

    kfc::test::SocketTestHooks::forced_read_message = std::string("play");
    server.tick_once();
    kfc::test::SocketTestHooks::reset();
    REQUIRE_EQ(server.matchmaking().waiting_count(), 1u);

    kfc::test::SocketTestHooks::forced_read_message = std::string("play extra");
    server.tick_once();
    kfc::test::SocketTestHooks::reset();

    CHECK_EQ(server.matchmaking().waiting_count(), 1u);
    CHECK_FALSE(server.room().active());
}

TEST_CASE("GameServerTest - SwallowsDuplicatePlayWhileSearching") {
    kfc::test::SocketTestHooks::reset();
    kfc::GameServer server{0, kfc::test::make_board({{"wK", ".", "bK"}}), ":memory:"};
    kfc::WebSocketClient client{"127.0.0.1", server.websocket_server().port()};

    login_client(server, client, "dup_play_user");

    kfc::test::SocketTestHooks::forced_read_message = std::string("play");
    server.tick_once();
    kfc::test::SocketTestHooks::reset();
    REQUIRE_EQ(server.matchmaking().waiting_count(), 1u);

    kfc::test::SocketTestHooks::forced_read_message = std::string("play");
    server.tick_once();
    kfc::test::SocketTestHooks::reset();

    CHECK_EQ(server.matchmaking().waiting_count(), 1u);
    CHECK_FALSE(server.room().active());
}

TEST_CASE("GameServerTest - MatchmakingTimeoutNotifiesClient") {
    kfc::GameServer server{0, kfc::test::make_board({{"wK", ".", "bK"}}), ":memory:"};
    kfc::WebSocketClient client{"127.0.0.1", server.websocket_server().port()};

    server.matchmaking().set_queue_timeout(std::chrono::milliseconds(0));
    login_client(server, client, "timeout_user");
    REQUIRE(client.try_send("play"));
    server.tick_once();

    bool saw_timeout = false;
    for (int attempt = 0; attempt < 100 && !saw_timeout; ++attempt) {
        if (const auto message = client.try_receive_snapshot()) {
            if (kfc::read_matchmaking_message(*message) == kfc::MatchmakingState::Timeout) {
                saw_timeout = true;
            }
        }
        server.tick_once();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    CHECK(saw_timeout);
}

TEST_CASE("GameServerTest - RejectsMalformedGameCommands") {
    kfc::GameServer server{0, kfc::test::make_board({{"wK", ".", "bK"}}), ":memory:"};
    kfc::WebSocketClient white_client{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient black_client{"127.0.0.1", server.websocket_server().port()};

    start_match(server, white_client, black_client, "cmd_white", "cmd_black");

    REQUIRE(white_client.try_send("   "));
    server.tick_once();
    REQUIRE(white_client.try_send("clear extra"));
    server.tick_once();
    REQUIRE(white_client.try_send("select 0"));
    server.tick_once();
    REQUIRE(white_client.try_send("move 0 1 2"));
    server.tick_once();
    REQUIRE(white_client.try_send("select bad col"));
    server.tick_once();
    REQUIRE(white_client.try_send("jump -1 0"));
    server.tick_once();
    CHECK(server.room().active());
}

TEST_CASE("GameServerTest - ProcessesJumpCommandForAssignedSide") {
    kfc::GameServer server{
        0, kfc::test::make_board({{"wK", "wN", "bK"}, {".", ".", "."}}), ":memory:"};
    kfc::WebSocketClient white_client{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient black_client{"127.0.0.1", server.websocket_server().port()};

    start_match(server, white_client, black_client, "jump_white", "jump_black");

    REQUIRE(white_client.try_send("select 0 1"));
    server.tick_once();
    REQUIRE(white_client.try_send("jump 0 1"));
    server.tick_once();
    CHECK(server.room().active());
}

TEST_CASE("GameServerTest - RejectsMoveSelectedForWrongSide") {
    kfc::GameServer server{0, kfc::test::make_board({{"wK", ".", "bK"}}), ":memory:"};
    kfc::WebSocketClient white_client{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient black_client{"127.0.0.1", server.websocket_server().port()};

    start_match(server, white_client, black_client, "side_white", "side_black");

    REQUIRE(white_client.try_send("select 0 0"));
    server.tick_once();
    REQUIRE(black_client.try_send("move 0 1"));
    server.tick_once();
    CHECK(server.room().active());
}

TEST_CASE("GameServerTest - ClampsLoserRatingAtZero") {
    kfc::GameServer server{
        0, kfc::test::make_board({{"wR", ".", "bK"}, {"wK", ".", "."}}), ":memory:"};
    kfc::WebSocketClient white_client{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient black_client{"127.0.0.1", server.websocket_server().port()};

    kfc::AuthenticationService auth{server.player_repository()};
    REQUIRE(auth.authenticate("clamp_winner", "testpass").success);
    REQUIRE(auth.authenticate("clamp_loser", "testpass").success);

    const auto winner = server.player_repository().find_by_username("clamp_winner");
    const auto loser = server.player_repository().find_by_username("clamp_loser");
    REQUIRE(winner.has_value());
    REQUIRE(loser.has_value());
    REQUIRE(server.player_repository().update_rating(winner->id(), 50));
    REQUIRE(server.player_repository().update_rating(loser->id(), 10));

    login_client(server, white_client, "clamp_winner", "testpass");
    login_client(server, black_client, "clamp_loser", "testpass");

    REQUIRE(white_client.try_send("play"));
    REQUIRE(black_client.try_send("play"));
    for (int attempt = 0; attempt < 50 && !server.room().active(); ++attempt) {
        server.tick_once();
        (void)poll_message(white_client);
        (void)poll_message(black_client);
    }
    REQUIRE(server.room().active());

    kfc::Match& match = server.room().match();
    match.submit_action(kfc::Select{0, 0});
    match.submit_action(kfc::MoveSelected{0, 2});
    while (!match.is_game_over()) {
        match.tick(kfc::kMoveDurationMs);
    }
    server.tick_once();

    const auto loser_after = server.player_repository().find_by_username("clamp_loser");
    REQUIRE(loser_after.has_value());
    CHECK_EQ(loser_after->rating(), 0);
}

TEST_CASE("GameServerTest - RejectsWrongPassword") {
    kfc::GameServer server{0, kfc::test::make_board({{"wK", ".", "bK"}}), ":memory:"};
    kfc::WebSocketClient registered_client{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient attacker_client{"127.0.0.1", server.websocket_server().port()};

    login_client(server, registered_client, "auth_user", "correct");
    registered_client.disconnect();
    for (int attempt = 0; attempt < 50; ++attempt) {
        server.tick_once();
    }

    connect_through_server(server, attacker_client);
    REQUIRE(attacker_client.try_send("login auth_user wrongpass"));

    bool saw_failure = false;
    for (int attempt = 0; attempt < 50 && !saw_failure; ++attempt) {
        server.tick_once();
        if (const auto message = poll_login_message(attacker_client)) {
            const auto result = kfc::read_login_message(*message);
            REQUIRE(result.has_value());
            if (result->status == kfc::LoginResultStatus::Failed) {
                CHECK_EQ(result->failure_reason, "invalid_password");
                saw_failure = true;
            }
        }
    }
    CHECK(saw_failure);
}

TEST_CASE("GameServerTest - RejectsMissingPassword") {
    kfc::GameServer server{0, kfc::test::make_board({{"wK", ".", "bK"}}), ":memory:"};
    kfc::WebSocketClient client{"127.0.0.1", server.websocket_server().port()};

    connect_through_server(server, client);
    REQUIRE(client.try_send("login no_password_user"));

    bool saw_failure = false;
    for (int attempt = 0; attempt < 50 && !saw_failure; ++attempt) {
        server.tick_once();
        if (const auto message = poll_login_message(client)) {
            const auto result = kfc::read_login_message(*message);
            REQUIRE(result.has_value());
            if (result->status == kfc::LoginResultStatus::Failed) {
                CHECK_EQ(result->failure_reason, "missing_password");
                saw_failure = true;
            }
        }
    }
    CHECK(saw_failure);
    CHECK_FALSE(server.player_repository().find_by_username("no_password_user").has_value());
}

TEST_CASE("GameServerTest - RejectsDuplicateLogin") {
    kfc::GameServer server{0, kfc::test::make_board({{"wK", ".", "bK"}}), ":memory:"};
    kfc::WebSocketClient first_client{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient second_client{"127.0.0.1", server.websocket_server().port()};

    login_client(server, first_client, "dup_user");

    connect_through_server(server, second_client);
    REQUIRE(second_client.try_send("login dup_user"));

    bool saw_failure = false;
    for (int attempt = 0; attempt < 50 && !saw_failure; ++attempt) {
        server.tick_once();
        if (const auto message = poll_login_message(second_client)) {
            const auto result = kfc::read_login_message(*message);
            REQUIRE(result.has_value());
            if (result->status == kfc::LoginResultStatus::Failed) {
                CHECK_EQ(result->failure_reason, "already_connected");
                saw_failure = true;
            }
        }
    }
    CHECK(saw_failure);
}

TEST_CASE("GameServerTest - RunLoopCanBeStoppedInTests") {
    kfc::GameServer server{0, kfc::test::make_board({{"wK", ".", "bK"}}), ":memory:"};

    std::thread server_thread{[&]() { server.run(); }};
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    server.request_stop();
    server_thread.join();
}
