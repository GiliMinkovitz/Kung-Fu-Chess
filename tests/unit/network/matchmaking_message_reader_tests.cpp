#include "network/matchmaking_message_reader.h"

#include <doctest/doctest.h>

TEST_CASE("MatchmakingMessageReaderTest - ParsesSearchingAndTimeout") {
    CHECK(kfc::read_matchmaking_message("searching") == kfc::MatchmakingState::Searching);
    CHECK(kfc::read_matchmaking_message("  searching  \n") ==
          kfc::MatchmakingState::Searching);
    CHECK(kfc::read_matchmaking_message("search_timeout") ==
          kfc::MatchmakingState::Timeout);
    CHECK(kfc::read_matchmaking_message("searching\r\n") ==
          kfc::MatchmakingState::Searching);
}

TEST_CASE("MatchmakingMessageReaderTest - ParsesMatchFoundSides") {
    CHECK(kfc::read_matchmaking_message("match_found white") ==
          kfc::MatchmakingState::MatchedWhite);
    CHECK(kfc::read_matchmaking_message("match_found black") ==
          kfc::MatchmakingState::MatchedBlack);
    CHECK_FALSE(kfc::read_matchmaking_message("match_found").has_value());
    CHECK_FALSE(kfc::read_matchmaking_message("match_found red").has_value());
}

TEST_CASE("MatchmakingMessageReaderTest - ParsesGameStartSides") {
    CHECK(kfc::read_matchmaking_message("game_start white") ==
          kfc::MatchmakingState::GameStartingWhite);
    CHECK(kfc::read_matchmaking_message("game_start black") ==
          kfc::MatchmakingState::GameStartingBlack);
    CHECK_FALSE(kfc::read_matchmaking_message("game_start").has_value());
}

TEST_CASE("MatchmakingMessageReaderTest - RejectsUnknownMessages") {
    CHECK_FALSE(kfc::read_matchmaking_message("").has_value());
    CHECK_FALSE(kfc::read_matchmaking_message("snapshot\nclock_ms 0").has_value());
    CHECK_FALSE(kfc::read_matchmaking_message("login alice").has_value());
}
