#include "core/piece.h"
#include "core/piece_factory.h"
#include "core/piece_token.h"

#include <doctest/doctest.h>

TEST_CASE("PieceTest - PieceFromTokenInvalid") {
    CHECK_FALSE(kfc::descriptor_from_token("xZ").has_value());
    CHECK_FALSE(kfc::descriptor_from_token("wX").has_value());
    CHECK_FALSE(kfc::descriptor_from_token("").has_value());
    CHECK_FALSE(kfc::descriptor_from_token("w").has_value());
    CHECK_FALSE(kfc::descriptor_from_token("abc").has_value());
}

TEST_CASE("PieceTest - PieceToToken") {
    CHECK_EQ(kfc::to_token(kfc::PieceColor::White, kfc::PieceKind::Rook), "wR");
    const std::optional<kfc::PieceDescriptor> descriptor = kfc::descriptor_from_token("bR");
    REQUIRE(descriptor.has_value());
    CHECK_EQ(kfc::to_token(descriptor->color, descriptor->kind), "bR");
}

TEST_CASE("PieceTest - PieceColorHelpers") {
    kfc::PieceFactory factory;
    const kfc::Piece white =
        factory.create(kfc::PieceColor::White, kfc::PieceKind::Knight, {0, 0});
    const kfc::Piece black =
        factory.create(kfc::PieceColor::Black, kfc::PieceKind::Knight, {1, 0});
    CHECK(white.is_white());
    CHECK_FALSE(white.is_black());
    CHECK(black.is_black());
    CHECK(white.is_same_color_as(white));
    CHECK_FALSE(white.is_same_color_as(black));
    CHECK(white.is_opponent_of(black));
    CHECK_FALSE(white.is_opponent_of(white));
    CHECK_NE(white, black);
    CHECK_EQ(white, white);
}

TEST_CASE("PieceTest - PieceIdentityById") {
    kfc::PieceFactory factory;
    const kfc::Piece first =
        factory.create(kfc::PieceColor::White, kfc::PieceKind::King, {0, 0});
    const kfc::Piece second =
        factory.create(kfc::PieceColor::White, kfc::PieceKind::King, {0, 1});
    CHECK_NE(first, second);
    CHECK_EQ(first.id, 1);
    CHECK_EQ(second.id, 2);
}

TEST_CASE("PieceTest - PieceStateDefaultsToIdle") {
    kfc::PieceFactory factory;
    const kfc::Piece piece =
        factory.create(kfc::PieceColor::White, kfc::PieceKind::Pawn, {0, 0});
    CHECK_EQ(piece.state, kfc::PieceState::Idle);
}
