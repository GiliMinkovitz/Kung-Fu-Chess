#include "network/game_result_message_reader.h"

#include <doctest/doctest.h>

TEST_CASE("GameResultMessageReaderTest - ParsesWinCheckmate") {
    const auto result = kfc::read_game_result_message("game_result win checkmate 1025");
    REQUIRE(result.has_value());
    CHECK(result->won);
    CHECK_EQ(result->reason, "checkmate");
    CHECK_EQ(result->rating, 1025);
}

TEST_CASE("GameResultMessageReaderTest - ParsesLossResign") {
    const auto result = kfc::read_game_result_message("game_result loss resign 975");
    REQUIRE(result.has_value());
    CHECK_FALSE(result->won);
    CHECK_EQ(result->reason, "resign");
    CHECK_EQ(result->rating, 975);
}

TEST_CASE("GameResultMessageReaderTest - ParsesOpponentDisconnect") {
    const auto result = kfc::read_game_result_message("game_result win opponent_disconnect 1012");
    REQUIRE(result.has_value());
    CHECK(result->won);
    CHECK_EQ(result->reason, "opponent_disconnect");
    CHECK_EQ(result->rating, 1012);
}

TEST_CASE("GameResultMessageReaderTest - IgnoresInvalidMessages") {
    CHECK_FALSE(kfc::read_game_result_message("").has_value());
    CHECK_FALSE(kfc::read_game_result_message("game_result win checkmate").has_value());
    CHECK_FALSE(kfc::read_game_result_message("game_result draw checkmate 1000").has_value());
    CHECK_FALSE(kfc::read_game_result_message("game_result win checkmate bad").has_value());
    CHECK_FALSE(kfc::read_game_result_message("snapshot").has_value());
}
