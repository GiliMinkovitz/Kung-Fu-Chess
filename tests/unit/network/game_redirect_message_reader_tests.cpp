#include "network/game_redirect_message_reader.h"

#include <doctest/doctest.h>

TEST_CASE("GameRedirectMessageReaderTest - ParsesRedirectMessage") {
    const std::optional<kfc::GameRedirectInfo> redirect =
        kfc::read_game_redirect_message("game_redirect ws://localhost:8765 42 white");

    REQUIRE(redirect.has_value());
    CHECK_EQ(redirect->endpoint, "ws://localhost:8765");
    CHECK_EQ(redirect->room_id, 42u);
    CHECK_EQ(redirect->side, kfc::PieceColor::White);
}

TEST_CASE("GameRedirectMessageReaderTest - ParsesBlackSide") {
    const std::optional<kfc::GameRedirectInfo> redirect =
        kfc::read_game_redirect_message("game_redirect ws://games.example:9000 7 black");

    REQUIRE(redirect.has_value());
    CHECK_EQ(redirect->endpoint, "ws://games.example:9000");
    CHECK_EQ(redirect->room_id, 7u);
    CHECK_EQ(redirect->side, kfc::PieceColor::Black);
}

TEST_CASE("GameRedirectMessageReaderTest - RejectsInvalidMessages") {
    CHECK_FALSE(kfc::read_game_redirect_message("").has_value());
    CHECK_FALSE(kfc::read_game_redirect_message("game_redirect").has_value());
    CHECK_FALSE(kfc::read_game_redirect_message("game_redirect ws://localhost:8765").has_value());
    CHECK_FALSE(kfc::read_game_redirect_message("game_redirect ws://localhost:8765 42 red")
                    .has_value());
    CHECK_FALSE(kfc::read_game_redirect_message("match_found white").has_value());
}
