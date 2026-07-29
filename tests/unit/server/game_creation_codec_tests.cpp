#include "server/game/protocol/game_creation_codec.h"

#include <doctest/doctest.h>

TEST_CASE("GameCreationCodecTest - RoundTripsRequest") {
    const kfc::GameCreationRequest request{10, 20, 99};
    const std::string encoded = kfc::encode_game_creation_request(request);
    const std::optional<kfc::GameCreationRequest> decoded =
        kfc::decode_game_creation_request(encoded);

    REQUIRE(decoded.has_value());
    CHECK_EQ(decoded->white_user_id, 10u);
    CHECK_EQ(decoded->black_user_id, 20u);
    REQUIRE(decoded->db_game_id.has_value());
    CHECK_EQ(*decoded->db_game_id, 99);
}

TEST_CASE("GameCreationCodecTest - RoundTripsRequestWithoutDbGameId") {
    const kfc::GameCreationRequest request{1, 2, std::nullopt};
    const std::string encoded = kfc::encode_game_creation_request(request);
    const std::optional<kfc::GameCreationRequest> decoded =
        kfc::decode_game_creation_request(encoded);

    REQUIRE(decoded.has_value());
    CHECK_EQ(decoded->white_user_id, 1u);
    CHECK_EQ(decoded->black_user_id, 2u);
    CHECK_FALSE(decoded->db_game_id.has_value());
}

TEST_CASE("GameCreationCodecTest - RoundTripsResponse") {
    const kfc::GameCreationResponse response{42, "game-server-1", std::string{"ws://games:8766"}};
    const std::string encoded = kfc::encode_game_creation_response(response);
    const std::optional<kfc::GameCreationResponse> decoded =
        kfc::decode_game_creation_response(encoded);

    REQUIRE(decoded.has_value());
    CHECK_EQ(decoded->room_id, 42u);
    CHECK_EQ(decoded->game_server_id, "game-server-1");
    REQUIRE(decoded->endpoint.has_value());
    CHECK_EQ(*decoded->endpoint, "ws://games:8766");
}

TEST_CASE("GameCreationCodecTest - RejectsMalformedRequest") {
    CHECK_FALSE(kfc::decode_game_creation_request("").has_value());
    CHECK_FALSE(kfc::decode_game_creation_request("bad").has_value());
    CHECK_FALSE(kfc::decode_game_creation_request("1|abc").has_value());
}

TEST_CASE("GameCreationCodecTest - RejectsMalformedResponse") {
    CHECK_FALSE(kfc::decode_game_creation_response("").has_value());
    CHECK_FALSE(kfc::decode_game_creation_response("room").has_value());
    CHECK_FALSE(kfc::decode_game_creation_response("abc|server|ws://x").has_value());
}
