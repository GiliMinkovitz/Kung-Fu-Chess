#include "app/database_config.h"
#include "app/server_infrastructure.h"
#include "model/game_config.h"
#include "network/game_result_message_reader.h"
#include "network/login_message_reader.h"
#include "network/matchmaking_message_reader.h"
#include "network/websocket_client.h"
#include "server/authentication_service.h"
#include "server/game_result_message_writer.h"
#include "server/game_server.h"
#include "server/rating_service.h"
#include "server/websocket_server.h"
#include "test/socket_test_hooks.h"
#include "test_game_server_fixture.h"
#include "test_game_server_helpers.h"
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
    std::thread accept_thread{[&]() {
        for (int attempt = 0; attempt < 1000 && server.websocket_server().clients().size() < expected_count;
             ++attempt) {
            server.tick_once();
            if (server.websocket_server().clients().size() < expected_count) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }};
    client.connect();
    accept_thread.join();
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

std::optional<std::string> drain_latest_snapshot(kfc::WebSocketClient& client) {
    std::optional<std::string> latest;
    for (int burst = 0; burst < 100; ++burst) {
        const auto message = client.try_receive_snapshot();
        if (!message) {
            break;
        }
        if (message->find("snapshot") != std::string::npos) {
            latest = std::move(message);
        }
    }
    return latest;
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

void drain_client_messages(kfc::WebSocketClient& client, int max_messages = 100) {
    int drained = 0;
    int idle_attempts = 0;
    while (drained < max_messages && idle_attempts < 20) {
        if (const auto message = client.try_receive_snapshot()) {
            ++drained;
            idle_attempts = 0;
            continue;
        }
        ++idle_attempts;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// Consumes every message currently pending on the client, keeping the first
// game_result found. Other message types are discarded.
void poll_available_game_results(kfc::WebSocketClient& client, std::optional<kfc::GameResult>& out) {
    if (out.has_value()) {
        return;
    }

    for (int burst = 0; burst < 100; ++burst) {
        const auto message = client.try_receive_snapshot();
        if (!message) {
            break;
        }
        if (const auto result = kfc::read_game_result_message(*message)) {
            out = result;
            break;
        }
    }
}

std::optional<kfc::GameResult> poll_game_result(kfc::WebSocketClient& client,
                                                int max_attempts = 2000) {
    std::optional<kfc::GameResult> result;
    for (int attempt = 0; attempt < max_attempts && !result.has_value(); ++attempt) {
        poll_available_game_results(client, result);
        if (!result.has_value()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    return result;
}

struct GameResultPair {
    std::optional<kfc::GameResult> white;
    std::optional<kfc::GameResult> black;
};

GameResultPair poll_both_game_results(kfc::WebSocketClient& white_client,
                                      kfc::WebSocketClient& black_client,
                                      int max_attempts = 2000) {
    GameResultPair results;
    for (int attempt = 0; attempt < max_attempts && (!results.white || !results.black);
         ++attempt) {
        poll_available_game_results(white_client, results.white);
        poll_available_game_results(black_client, results.black);
        if (!results.white || !results.black) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    return results;
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

    kfc::Room* room = nullptr;
    for (int attempt = 0; attempt < 50; ++attempt) {
        room = kfc::test::find_room_for_player(server, white_name);
        if (room != nullptr && room->active()) {
            break;
        }
        server.tick_once();
        (void)white_client.try_receive_snapshot();
        (void)black_client.try_receive_snapshot();
    }

    REQUIRE(room != nullptr);
    REQUIRE(room->active());
    REQUIRE(room->db_game_id().has_value());

    drain_client_messages(white_client, 200);
    drain_client_messages(black_client, 200);
}

}  // namespace

TEST_CASE("GameServerTest - InitializesInMemoryDatabase") {
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;

    CHECK(server.database().connection() != nullptr);
    CHECK(server.user_repository().find_by_username("missing") == nullptr);
}

TEST_CASE("GameServerTest - RejectsInvalidDatabasePath") {
    kfc::app::DatabaseConfig invalid_config;
    invalid_config.path = "/tmp";
    CHECK_THROWS_AS((kfc::app::ServerInfrastructure{invalid_config}), std::runtime_error);
}

TEST_CASE("GameServerTest - AcceptsClientAndProcessesLogin") {
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;
    kfc::WebSocketClient client{"127.0.0.1", server.websocket_server().port()};

    login_client(server, client, "server_user");

    const auto player = server.user_repository().find_profile_by_id(
        server.user_repository().find_by_username("server_user")->id());
    REQUIRE(player.has_value());
    CHECK_EQ(player->rating(), 1000);

    const kfc::User* user = server.user_repository().find_by_username("server_user");
    REQUIRE(user != nullptr);
    CHECK_EQ(user->id(), static_cast<kfc::UserId>(player->id()));
    CHECK_EQ(user->username(), "server_user");
}

TEST_CASE("GameServerTest - CreatedRoomStoresSessionBindingsAndDbGameId") {
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;
    kfc::WebSocketClient white_client{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient black_client{"127.0.0.1", server.websocket_server().port()};

    start_match(server, white_client, black_client, "bound_white", "bound_black");

    kfc::Room* room = kfc::test::find_room_for_player(server, "bound_white");
    REQUIRE(room != nullptr);
    REQUIRE(room->white_session() != nullptr);
    REQUIRE(room->black_session() != nullptr);
    REQUIRE(room->db_game_id().has_value());
    CHECK_EQ(room->white_session()->player().username(), "bound_white");
    CHECK_EQ(room->black_session()->player().username(), "bound_black");
    CHECK(room->white_session()->has_side());
    CHECK(room->black_session()->has_side());
}

TEST_CASE("GameServerTest - MatchmakingFlowFindsOpponents") {
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;
    kfc::WebSocketClient white_client{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient black_client{"127.0.0.1", server.websocket_server().port()};

    login_client(server, white_client, "white_user");
    REQUIRE(white_client.try_send("play"));
    for (int attempt = 0; attempt < 50 && server.matchmaking_service().waiting_count() == 0; ++attempt) {
        server.tick_once();
    }
    REQUIRE_EQ(server.matchmaking_service().waiting_count(), 1u);

    login_and_queue(server, black_client, "black_user");
    kfc::Room* room = nullptr;
    for (int attempt = 0; attempt < 50; ++attempt) {
        room = kfc::test::find_room_for_player(server, "white_user");
        if (room != nullptr && room->active()) {
            break;
        }
        server.tick_once();
        (void)poll_message(white_client);
        (void)poll_message(black_client);
    }

    REQUIRE(room != nullptr);
    CHECK(room->active());
    REQUIRE(room->db_game_id().has_value());
}

TEST_CASE("GameServerTest - ActiveRoomBroadcastsSnapshotsAndProcessesMoves") {
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;
    kfc::WebSocketClient white_client{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient black_client{"127.0.0.1", server.websocket_server().port()};

    start_match(server, white_client, black_client, "active_white", "active_black");

    bool snapshots_matched = false;
    for (int attempt = 0; attempt < 200 && !snapshots_matched; ++attempt) {
        server.tick_once();
        const auto white_snapshot = drain_latest_snapshot(white_client);
        const auto black_snapshot = drain_latest_snapshot(black_client);
        if (white_snapshot.has_value() && black_snapshot.has_value()) {
            CHECK(white_snapshot->find("snapshot") != std::string::npos);
            if (*white_snapshot == *black_snapshot) {
                snapshots_matched = true;
            }
        }
    }
    REQUIRE(snapshots_matched);

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
    kfc::test::GameServerFixture fixture{
        0, kfc::test::make_board({{"wR", ".", "bK"}, {"wK", ".", "."}})};
    kfc::GameServer& server = fixture.server;
    kfc::WebSocketClient white_client{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient black_client{"127.0.0.1", server.websocket_server().port()};

    start_match(server, white_client, black_client, "winner", "loser");

    const auto winner = kfc::test::find_player_profile(server.user_repository(), "winner");
    const auto loser = kfc::test::find_player_profile(server.user_repository(), "loser");
    REQUIRE(winner.has_value());
    REQUIRE(loser.has_value());
    const int winner_before = winner->rating();
    const int loser_before = loser->rating();
    const kfc::RatingChange expected =
        kfc::RatingService{}.calculate(winner_before, loser_before);
    kfc::Room* room = kfc::test::find_room_for_player(server, "winner");
    REQUIRE(room != nullptr);
    const int game_id = *room->db_game_id();

    kfc::Match& match = room->match();
    match.submit_action(kfc::Select{0, 0});
    match.submit_action(kfc::MoveSelected{0, 2});

    for (int ticks = 0; !match.is_game_over() && ticks < 10000; ++ticks) {
        match.tick(kfc::kMoveDurationMs);
    }
    REQUIRE(match.is_game_over());
    server.tick_once();

    REQUIRE(server.room_manager().active_rooms().empty());

    const auto winner_after = kfc::test::find_player_profile(server.user_repository(), "winner");
    const auto loser_after = kfc::test::find_player_profile(server.user_repository(), "loser");
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

TEST_CASE("GameServerTest - FinishesGameWithExplicitWinner") {
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;
    kfc::WebSocketClient white_client{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient black_client{"127.0.0.1", server.websocket_server().port()};

    start_match(server, white_client, black_client, "explicit_winner", "explicit_loser");

    const auto winner = kfc::test::find_player_profile(server.user_repository(), "explicit_winner");
    const auto loser = kfc::test::find_player_profile(server.user_repository(), "explicit_loser");
    REQUIRE(winner.has_value());
    REQUIRE(loser.has_value());
    const kfc::RatingChange expected =
        kfc::RatingService{}.calculate(winner->rating(), loser->rating());
    kfc::Room* room = kfc::test::find_room_for_player(server, "explicit_winner");
    REQUIRE(room != nullptr);
    const int game_id = *room->db_game_id();

    CHECK_FALSE(room->match().is_game_over());

    server.finish_room(room->id(), kfc::PieceColor::White, kfc::FinishReason::Disconnect);

    REQUIRE(server.room_manager().active_rooms().empty());

    const auto winner_after =
        kfc::test::find_player_profile(server.user_repository(), "explicit_winner");
    const auto loser_after =
        kfc::test::find_player_profile(server.user_repository(), "explicit_loser");
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

TEST_CASE("GameServerTest - DisconnectingPlayerLosesGame") {
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;
    kfc::WebSocketClient white_client{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient black_client{"127.0.0.1", server.websocket_server().port()};

    start_match(server, white_client, black_client, "dc_white", "dc_black");

    const auto white = kfc::test::find_player_profile(server.user_repository(), "dc_white");
    const auto black = kfc::test::find_player_profile(server.user_repository(), "dc_black");
    REQUIRE(white.has_value());
    REQUIRE(black.has_value());
    const kfc::RatingChange expected =
        kfc::RatingService{}.calculate(black->rating(), white->rating());
    kfc::Room* room = kfc::test::find_room_for_player(server, "dc_white");
    REQUIRE(room != nullptr);
    const int game_id = *room->db_game_id();

    white_client.disconnect();
    server.tick_once();

    REQUIRE(server.room_manager().active_rooms().empty());

    const auto black_after = kfc::test::find_player_profile(server.user_repository(), "dc_black");
    const auto white_after = kfc::test::find_player_profile(server.user_repository(), "dc_white");
    REQUIRE(black_after.has_value());
    REQUIRE(white_after.has_value());
    CHECK_EQ(black_after->rating(), expected.winner_new_rating);
    CHECK_EQ(white_after->rating(), expected.loser_new_rating);

    sqlite3* db = server.database().connection();
    sqlite3_stmt* stmt = nullptr;
    REQUIRE(sqlite3_prepare_v2(db,
                               "SELECT status, winner_id FROM games WHERE id = ? LIMIT 1;", -1,
                               &stmt, nullptr) == SQLITE_OK);
    sqlite3_bind_int(stmt, 1, game_id);
    REQUIRE(sqlite3_step(stmt) == SQLITE_ROW);
    CHECK_EQ(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))),
             "finished");
    CHECK_EQ(sqlite3_column_int(stmt, 1), black_after->id());
    sqlite3_finalize(stmt);
}

TEST_CASE("GameServerTest - ResignFinishesGame") {
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;
    kfc::WebSocketClient white_client{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient black_client{"127.0.0.1", server.websocket_server().port()};

    start_match(server, white_client, black_client, "resign_white", "resign_black");

    const auto white = kfc::test::find_player_profile(server.user_repository(), "resign_white");
    const auto black = kfc::test::find_player_profile(server.user_repository(), "resign_black");
    REQUIRE(white.has_value());
    REQUIRE(black.has_value());
    const kfc::RatingChange expected =
        kfc::RatingService{}.calculate(black->rating(), white->rating());
    kfc::Room* room = kfc::test::find_room_for_player(server, "resign_white");
    REQUIRE(room != nullptr);
    const int game_id = *room->db_game_id();

    REQUIRE(white_client.try_send("resign"));
    server.tick_once();

    REQUIRE(server.room_manager().active_rooms().empty());

    const auto black_after =
        kfc::test::find_player_profile(server.user_repository(), "resign_black");
    const auto white_after =
        kfc::test::find_player_profile(server.user_repository(), "resign_white");
    REQUIRE(black_after.has_value());
    REQUIRE(white_after.has_value());
    CHECK_EQ(black_after->rating(), expected.winner_new_rating);
    CHECK_EQ(white_after->rating(), expected.loser_new_rating);

    sqlite3* db = server.database().connection();
    sqlite3_stmt* stmt = nullptr;
    REQUIRE(sqlite3_prepare_v2(db,
                               "SELECT status, winner_id FROM games WHERE id = ? LIMIT 1;", -1,
                               &stmt, nullptr) == SQLITE_OK);
    sqlite3_bind_int(stmt, 1, game_id);
    REQUIRE(sqlite3_step(stmt) == SQLITE_ROW);
    CHECK_EQ(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))),
             "finished");
    CHECK_EQ(sqlite3_column_int(stmt, 1), black_after->id());
    sqlite3_finalize(stmt);
}

TEST_CASE("GameServerTest - DeliversGameResultMessageOverWebSocket") {
    kfc::WebSocketServer server{0};
    kfc::WebSocketClient client{"127.0.0.1", server.port()};

    const std::size_t expected_count = server.clients().size() + 1;
    std::thread accept_thread{[&]() {
        for (int attempt = 0; attempt < 1000 && server.clients().size() < expected_count; ++attempt) {
            server.try_accept();
            if (server.clients().size() < expected_count) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }};
    client.connect();
    accept_thread.join();
    REQUIRE_EQ(server.clients().size(), expected_count);

    const std::string message =
        kfc::create_game_result_message(true, kfc::FinishReason::Resign, 1025);
    REQUIRE(server.clients().back().try_send(message));

    const auto received = poll_game_result(client);
    REQUIRE(received.has_value());
    CHECK(received->won);
    CHECK_EQ(received->reason, "resign");
    CHECK_EQ(received->rating, 1025);
}

TEST_CASE("GameServerTest - GameResultSentOnExplicitFinish") {
    kfc::test::SocketTestHooks::reset();
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;
    kfc::WebSocketClient white_client{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient black_client{"127.0.0.1", server.websocket_server().port()};

    start_match(server, white_client, black_client, "result_explicit_white", "result_explicit_black");

    const kfc::RatingChange expected = kfc::RatingService{}.calculate(1000, 1000);
    kfc::Room* room = kfc::test::find_room_for_player(server, "result_explicit_white");
    REQUIRE(room != nullptr);

    server.finish_room(room->id(), kfc::PieceColor::White, kfc::FinishReason::Resign);

    const GameResultPair results = poll_both_game_results(white_client, black_client);

    REQUIRE(results.white.has_value());
    REQUIRE(results.black.has_value());
    CHECK(results.white->won);
    CHECK_FALSE(results.black->won);
    CHECK_EQ(results.white->reason, "resign");
    CHECK_EQ(results.white->rating, expected.winner_new_rating);
    CHECK_EQ(results.black->rating, expected.loser_new_rating);
}

TEST_CASE("GameServerTest - GameResultSentAfterResign") {
    kfc::test::SocketTestHooks::reset();
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;
    kfc::WebSocketClient white_client{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient black_client{"127.0.0.1", server.websocket_server().port()};

    start_match(server, white_client, black_client, "result_resign_white", "result_resign_black");

    const auto white =
        kfc::test::find_player_profile(server.user_repository(), "result_resign_white");
    const auto black =
        kfc::test::find_player_profile(server.user_repository(), "result_resign_black");
    REQUIRE(white.has_value());
    REQUIRE(black.has_value());
    const kfc::RatingChange expected =
        kfc::RatingService{}.calculate(black->rating(), white->rating());

    REQUIRE(white_client.try_send("resign"));
    server.tick_once();

    const GameResultPair results = poll_both_game_results(white_client, black_client);

    REQUIRE(results.black.has_value());
    REQUIRE(results.white.has_value());
    CHECK(results.black->won);
    CHECK_EQ(results.black->reason, "resign");
    CHECK_EQ(results.black->rating, expected.winner_new_rating);
    CHECK_FALSE(results.white->won);
    CHECK_EQ(results.white->reason, "resign");
    CHECK_EQ(results.white->rating, expected.loser_new_rating);
}

TEST_CASE("GameServerTest - GameResultSentAfterDisconnect") {
    kfc::test::SocketTestHooks::reset();
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;
    kfc::WebSocketClient white_client{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient black_client{"127.0.0.1", server.websocket_server().port()};

    start_match(server, white_client, black_client, "result_dc_white", "result_dc_black");

    const auto white = kfc::test::find_player_profile(server.user_repository(), "result_dc_white");
    const auto black = kfc::test::find_player_profile(server.user_repository(), "result_dc_black");
    REQUIRE(white.has_value());
    REQUIRE(black.has_value());
    const kfc::RatingChange expected =
        kfc::RatingService{}.calculate(black->rating(), white->rating());

    white_client.disconnect();
    server.tick_once();

    const std::optional<kfc::GameResult> black_result = poll_game_result(black_client, 2000);

    REQUIRE(black_result.has_value());
    CHECK(black_result->won);
    CHECK_EQ(black_result->reason, "opponent_disconnect");
    CHECK_EQ(black_result->rating, expected.winner_new_rating);
}

TEST_CASE("GameServerTest - ExposesRepositoriesAndMatchmaking") {
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;

    CHECK(server.database().connection() != nullptr);
    CHECK_EQ(server.matchmaking_service().waiting_count(), 0u);
    CHECK(&server.game_repository() == &server.game_repository());
}

TEST_CASE("GameServerTest - RejectsMalformedLoginAndPlayMessages") {
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;
    kfc::WebSocketClient client{"127.0.0.1", server.websocket_server().port()};

    connect_through_server(server, client);

    REQUIRE(client.try_send("   "));
    server.tick_once();
    CHECK(server.user_repository().find_by_username("anyone") == nullptr);

    REQUIRE(client.try_send("login"));
    server.tick_once();
    CHECK(server.user_repository().find_by_username("anyone") == nullptr);

    REQUIRE(client.try_send("login malformed_alice extra token"));
    server.tick_once();
    CHECK(server.user_repository().find_by_username("malformed_alice") == nullptr);

    REQUIRE(client.try_send("signin nobody"));
    server.tick_once();
    CHECK(server.user_repository().find_by_username("nobody") == nullptr);

    kfc::WebSocketClient valid_client{"127.0.0.1", server.websocket_server().port()};
    login_client(server, valid_client, "valid_play_user");
    REQUIRE(server.user_repository().find_by_username("valid_play_user") != nullptr);

    REQUIRE(valid_client.try_send("play extra"));
    server.tick_once();
    CHECK(server.room_manager().active_rooms().empty());

    REQUIRE(valid_client.try_send("play"));
    server.tick_once();
    CHECK(server.room_manager().active_rooms().empty());
}

TEST_CASE("GameServerTest - IgnoresPlayMessagesWhileSearching") {
    kfc::test::SocketTestHooks::reset();
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;
    kfc::WebSocketClient client{"127.0.0.1", server.websocket_server().port()};

    login_client(server, client, "queue_user");

    kfc::test::SocketTestHooks::forced_read_message = std::string("play");
    server.tick_once();
    kfc::test::SocketTestHooks::reset();
    REQUIRE_EQ(server.matchmaking_service().waiting_count(), 1u);

    kfc::test::SocketTestHooks::forced_read_message = std::string("play extra");
    server.tick_once();
    kfc::test::SocketTestHooks::reset();

    CHECK_EQ(server.matchmaking_service().waiting_count(), 1u);
    CHECK(server.room_manager().active_rooms().empty());
}

TEST_CASE("GameServerTest - SwallowsDuplicatePlayWhileSearching") {
    kfc::test::SocketTestHooks::reset();
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;
    kfc::WebSocketClient client{"127.0.0.1", server.websocket_server().port()};

    login_client(server, client, "dup_play_user");

    kfc::test::SocketTestHooks::forced_read_message = std::string("play");
    server.tick_once();
    kfc::test::SocketTestHooks::reset();
    REQUIRE_EQ(server.matchmaking_service().waiting_count(), 1u);

    kfc::test::SocketTestHooks::forced_read_message = std::string("play");
    server.tick_once();
    kfc::test::SocketTestHooks::reset();

    CHECK_EQ(server.matchmaking_service().waiting_count(), 1u);
    CHECK(server.room_manager().active_rooms().empty());
}

TEST_CASE("GameServerTest - MatchmakingTimeoutNotifiesClient") {
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;
    kfc::WebSocketClient client{"127.0.0.1", server.websocket_server().port()};

    server.matchmaking_service().set_queue_timeout(std::chrono::milliseconds(0));
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
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;
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
    kfc::Room* room = kfc::test::find_room_for_player(server, "cmd_white");
    REQUIRE(room != nullptr);
    CHECK(room->active());
}

TEST_CASE("GameServerTest - ProcessesJumpCommandForAssignedSide") {
    kfc::test::GameServerFixture fixture{
        0, kfc::test::make_board({{"wK", "wN", "bK"}, {".", ".", "."}})};
    kfc::GameServer& server = fixture.server;
    kfc::WebSocketClient white_client{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient black_client{"127.0.0.1", server.websocket_server().port()};

    start_match(server, white_client, black_client, "jump_white", "jump_black");

    REQUIRE(white_client.try_send("select 0 1"));
    server.tick_once();
    REQUIRE(white_client.try_send("jump 0 1"));
    server.tick_once();
    kfc::Room* room = kfc::test::find_room_for_player(server, "jump_white");
    REQUIRE(room != nullptr);
    CHECK(room->active());
}

TEST_CASE("GameServerTest - RejectsMoveSelectedForWrongSide") {
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;
    kfc::WebSocketClient white_client{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient black_client{"127.0.0.1", server.websocket_server().port()};

    start_match(server, white_client, black_client, "side_white", "side_black");

    REQUIRE(white_client.try_send("select 0 0"));
    server.tick_once();
    REQUIRE(black_client.try_send("move 0 1"));
    server.tick_once();
    kfc::Room* room = kfc::test::find_room_for_player(server, "side_white");
    REQUIRE(room != nullptr);
    CHECK(room->active());
}

TEST_CASE("GameServerTest - ClampsLoserRatingAtZero") {
    kfc::test::GameServerFixture fixture{
        0, kfc::test::make_board({{"wR", ".", "bK"}, {"wK", ".", "."}})};
    kfc::GameServer& server = fixture.server;
    kfc::WebSocketClient white_client{"127.0.0.1", server.websocket_server().port()};
    kfc::WebSocketClient black_client{"127.0.0.1", server.websocket_server().port()};

    kfc::AuthenticationService auth{server.user_repository()};
    REQUIRE(auth.authenticate("clamp_winner", "testpass").success);
    REQUIRE(auth.authenticate("clamp_loser", "testpass").success);

    const auto winner = server.user_repository().find_profile_by_id(
        server.user_repository().find_by_username("clamp_winner")->id());
    const auto loser = server.user_repository().find_profile_by_id(
        server.user_repository().find_by_username("clamp_loser")->id());
    REQUIRE(winner.has_value());
    REQUIRE(loser.has_value());
    REQUIRE(server.user_repository().update_rating(static_cast<kfc::UserId>(winner->id()), 50));
    REQUIRE(server.user_repository().update_rating(static_cast<kfc::UserId>(loser->id()), 10));

    login_client(server, white_client, "clamp_winner", "testpass");
    login_client(server, black_client, "clamp_loser", "testpass");

    REQUIRE(white_client.try_send("play"));
    REQUIRE(black_client.try_send("play"));
    kfc::Room* room = nullptr;
    for (int attempt = 0; attempt < 50; ++attempt) {
        room = kfc::test::find_room_for_player(server, "clamp_winner");
        if (room != nullptr && room->active()) {
            break;
        }
        server.tick_once();
        (void)poll_message(white_client);
        (void)poll_message(black_client);
    }
    REQUIRE(room != nullptr);
    REQUIRE(room->active());

    kfc::Match& match = room->match();
    match.submit_action(kfc::Select{0, 0});
    match.submit_action(kfc::MoveSelected{0, 2});
    for (int ticks = 0; !match.is_game_over() && ticks < 10000; ++ticks) {
        match.tick(kfc::kMoveDurationMs);
    }
    REQUIRE(match.is_game_over());
    server.tick_once();

    const auto loser_after =
        kfc::test::find_player_profile(server.user_repository(), "clamp_loser");
    REQUIRE(loser_after.has_value());
    CHECK_EQ(loser_after->rating(), 0);
}

TEST_CASE("GameServerTest - RejectsWrongPassword") {
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;
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
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;
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
    CHECK(server.user_repository().find_by_username("no_password_user") == nullptr);
}

TEST_CASE("GameServerTest - RejectsDuplicateLogin") {
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;
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
    kfc::test::GameServerFixture fixture{0, kfc::test::make_board({{"wK", ".", "bK"}})};
    kfc::GameServer& server = fixture.server;

    std::thread server_thread{[&]() { server.run(); }};
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    server.request_stop();
    server_thread.join();
}
