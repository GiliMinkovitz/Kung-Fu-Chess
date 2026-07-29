#include "database/player_repository.h"
#include "database/sqlite_database.h"
#include "logic/game_action.h"
#include "network/websocket_client.h"
#include "server/game_message_parser.h"
#include "server/match.h"
#include "server/player_session.h"
#include "server/websocket_server.h"
#include "test/game_message_parser_test_hooks.h"
#include "test_helpers.h"

#include <doctest/doctest.h>

#include <chrono>
#include <memory>
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

struct ParserTestSession {
    kfc::SqliteDatabase db{":memory:"};
    kfc::PlayerRepository repo;
    kfc::WebSocketServer server{0};
    std::unique_ptr<kfc::WebSocketClient> client;
    std::unique_ptr<kfc::PlayerSession> session;

    explicit ParserTestSession(const std::string& username)
        : repo(db) {
        REQUIRE(db.open());
        REQUIRE(db.initialize_schema());
        client = std::make_unique<kfc::WebSocketClient>("127.0.0.1", server.port());
        std::thread accept_thread{[&]() { accept_one_client(server); }};
        client->connect();
        accept_thread.join();
        session = std::make_unique<kfc::PlayerSession>(0, &server.clients().back());
        if (const auto player = repo.find_by_username(username)) {
            session->bind_player(*player);
        } else if (const auto created = repo.create_player(username, 1000)) {
            session->bind_player(*created);
        }
    }
};

}  // namespace

TEST_CASE("GameMessageParserTest - ParsePlayMessage") {
    CHECK(kfc::parse_play_message("play"));
    CHECK(kfc::parse_play_message("  play  "));
    CHECK_FALSE(kfc::parse_play_message(""));
    CHECK_FALSE(kfc::parse_play_message("   "));
    CHECK_FALSE(kfc::parse_play_message("play extra"));
    CHECK_FALSE(kfc::parse_play_message("queue"));
    CHECK_FALSE(kfc::parse_play_message(" play\textra "));
}

TEST_CASE("GameMessageParserTest - ParseJoinGameMessage") {
    CHECK_EQ(kfc::parse_join_game_message("join_game 42"), 42u);
    CHECK_EQ(kfc::parse_join_game_message("  join_game 7  "), 7u);
    CHECK_FALSE(kfc::parse_join_game_message("join_game").has_value());
    CHECK_FALSE(kfc::parse_join_game_message("join_game abc").has_value());
    CHECK_FALSE(kfc::parse_join_game_message("join_game 1 extra").has_value());
    CHECK_FALSE(kfc::parse_join_game_message("play").has_value());
}

TEST_CASE("GameMessageParserTest - ParseResignMessage") {
    CHECK(kfc::parse_resign_message("resign"));
    CHECK(kfc::parse_resign_message("  resign  "));
    CHECK_FALSE(kfc::parse_resign_message(""));
    CHECK_FALSE(kfc::parse_resign_message("   "));
    CHECK_FALSE(kfc::parse_resign_message("resign extra"));
    CHECK_FALSE(kfc::parse_resign_message("quit"));
}

TEST_CASE("GameMessageParserTest - ParseLoginMessage") {
    const auto alice = kfc::parse_login_message("login alice");
    REQUIRE(alice.has_value());
    CHECK(alice->username == "alice");
    CHECK(alice->password.empty());

    const auto with_password = kfc::parse_login_message("login bob secret");
    REQUIRE(with_password.has_value());
    CHECK(with_password->username == "bob");
    CHECK(with_password->password == "secret");

    const auto trimmed = kfc::parse_login_message("  login carol  ");
    REQUIRE(trimmed.has_value());
    CHECK(trimmed->username == "carol");

    CHECK_FALSE(kfc::parse_login_message("").has_value());
    CHECK_FALSE(kfc::parse_login_message("login").has_value());
    CHECK_FALSE(kfc::parse_login_message("login  ").has_value());
    CHECK_FALSE(kfc::parse_login_message("signin alice").has_value());
    CHECK_FALSE(kfc::parse_login_message("login alice extra token").has_value());
}

TEST_CASE("GameMessageParserTest - ParseGameCommands") {
    const auto clear = kfc::parse_message("clear");
    REQUIRE(clear.has_value());
    CHECK(std::holds_alternative<kfc::ClearSelection>(*clear));

    const auto select = kfc::parse_message("select 1 2");
    REQUIRE(select.has_value());
    REQUIRE(std::holds_alternative<kfc::Select>(*select));
    CHECK(std::get<kfc::Select>(*select).row == 1);
    CHECK(std::get<kfc::Select>(*select).col == 2);

    const auto move = kfc::parse_message("  move 0 3  ");
    REQUIRE(move.has_value());
    REQUIRE(std::holds_alternative<kfc::MoveSelected>(*move));

    const auto jump = kfc::parse_message("jump 2 2");
    REQUIRE(jump.has_value());
    REQUIRE(std::holds_alternative<kfc::JumpAt>(*jump));

    CHECK_FALSE(kfc::parse_message("").has_value());
    CHECK_FALSE(kfc::parse_message("   ").has_value());
    CHECK_FALSE(kfc::parse_message("clear extra").has_value());
    CHECK_FALSE(kfc::parse_message("select 0").has_value());
    CHECK_FALSE(kfc::parse_message("move 0 1 2").has_value());
    CHECK_FALSE(kfc::parse_message("select bad col").has_value());
    CHECK_FALSE(kfc::parse_message("jump -1 0").has_value());
    CHECK_FALSE(kfc::parse_message("advance 100").has_value());
}

TEST_CASE("GameMessageParserTest - IsActionAllowed") {
    kfc::Match match(kfc::test::make_board({{"wK", ".", "bK"}}));
    ParserTestSession white_session{"parser_white"};
    white_session.session->set_side(kfc::PieceColor::White);

    CHECK(kfc::is_action_allowed(*white_session.session, match, kfc::ClearSelection{}));
    CHECK_FALSE(kfc::is_action_allowed(*white_session.session, match, kfc::AdvanceClock{100}));
    CHECK(kfc::is_action_allowed(*white_session.session, match, kfc::Select{0, 0}));
    CHECK_FALSE(kfc::is_action_allowed(*white_session.session, match, kfc::Select{0, 2}));
    CHECK_FALSE(kfc::is_action_allowed(*white_session.session, match, kfc::Select{0, 1}));
    CHECK_FALSE(kfc::is_action_allowed(*white_session.session, match, kfc::MoveSelected{0, 1}));

    match.submit_action(kfc::Select{0, 0});
    CHECK(kfc::is_action_allowed(*white_session.session, match, kfc::MoveSelected{0, 1}));
    CHECK(kfc::is_action_allowed(*white_session.session, match, kfc::JumpSelected{}));

    ParserTestSession no_side_session{"parser_noside"};
    CHECK_FALSE(kfc::is_action_allowed(*no_side_session.session, match, kfc::ClearSelection{}));
}

TEST_CASE("GameMessageParserTest - RejectsEnemySelectionForMoveSelected") {
    kfc::Match match(kfc::test::make_board({{"wK", ".", "bK"}}));
    ParserTestSession black_session{"parser_black"};
    black_session.session->set_side(kfc::PieceColor::Black);

    match.submit_action(kfc::Select{0, 0});
    CHECK_FALSE(kfc::is_action_allowed(*black_session.session, match, kfc::MoveSelected{0, 1}));
}

TEST_CASE("GameMessageParserTest - ParseNonNegativeIntRejectsEmptyToken") {
    CHECK_FALSE(kfc::test::parse_non_negative_int_for_tests("").has_value());
}

TEST_CASE("GameMessageParserTest - RejectsInvalidPieceDescriptor") {
    kfc::Match match(kfc::test::make_board({{"wK", ".", "bK"}}));
    ParserTestSession white_session{"descriptor_white"};
    white_session.session->set_side(kfc::PieceColor::White);

    kfc::test::GameMessageParserTestHooks::force_invalid_piece_descriptor = true;
    CHECK_FALSE(kfc::is_action_allowed(*white_session.session, match, kfc::Select{0, 0}));
    kfc::test::GameMessageParserTestHooks::reset();
}
