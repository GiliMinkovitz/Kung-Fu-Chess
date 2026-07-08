#include "piece.h"

#include <doctest/doctest.h>
#include <optional>

TEST_CASE("PieceTest - PieceFromTokenInvalid") {
    CHECK_FALSE(kfc::Piece::from_token("xZ").has_value());
    CHECK_FALSE(kfc::Piece::from_token("wX").has_value());
    CHECK_FALSE(kfc::Piece::from_token("").has_value());
    CHECK_FALSE(kfc::Piece::from_token("w").has_value());
    CHECK_FALSE(kfc::Piece::from_token("abc").has_value());
}

TEST_CASE("PieceTest - PieceToToken") {
    CHECK_EQ(kfc::Piece::empty().to_token(), ".");
    const std::optional<kfc::Piece> piece = kfc::Piece::from_token("bR");
    REQUIRE(piece.has_value());
    CHECK_EQ(piece->to_token(), "bR");
}

TEST_CASE("PieceTest - PieceColorHelpers") {
    const kfc::Piece white = *kfc::Piece::from_token("wN");
    const kfc::Piece black = *kfc::Piece::from_token("bN");
    const kfc::Piece empty = kfc::Piece::empty();
    CHECK(white.is_white());
    CHECK_FALSE(white.is_black());
    CHECK(black.is_black());
    CHECK(white.is_same_color_as(white));
    CHECK_FALSE(white.is_same_color_as(black));
    CHECK_FALSE(white.is_same_color_as(empty));
    CHECK(white.is_opponent_of(black));
    CHECK_FALSE(white.is_opponent_of(white));
    CHECK_NE(white, black);
    CHECK_EQ(white, white);
}

TEST_CASE("PieceTest - PieceOpponentOfEmpty") {
    const kfc::Piece white = *kfc::Piece::from_token("wK");
    const kfc::Piece empty = kfc::Piece::empty();
    CHECK_FALSE(white.is_opponent_of(empty));
    CHECK_FALSE(empty.is_opponent_of(white));
}
