#include "ui/view/piece_sprite_selector.h"

#include <doctest/doctest.h>

TEST_CASE("PieceSpriteSelectorTest - SelectsIdleByDefault") {
    const kfc::PieceSpriteSelector selector;
    const kfc::PieceSpriteContext context{};

    const kfc::PieceSpriteSelection selection = selector.select(context);

    CHECK(selection.state == "idle");
    CHECK_EQ(selection.frame, 1);
}

TEST_CASE("PieceSpriteSelectorTest - SelectsMoveAnimationFrames") {
    const kfc::PieceSpriteSelector selector;
    kfc::PieceSpriteContext context{};
    context.moving = true;
    context.progress = 0.5f;

    const kfc::PieceSpriteSelection selection = selector.select(context);

    CHECK(selection.state == "move");
    CHECK_EQ(selection.frame, 3);
}

TEST_CASE("PieceSpriteSelectorTest - SelectsJumpAnimationFrames") {
    const kfc::PieceSpriteSelector selector;
    kfc::PieceSpriteContext context{};
    context.jumping = true;
    context.progress = 1.5f;

    const kfc::PieceSpriteSelection selection = selector.select(context);

    CHECK(selection.state == "jump");
    CHECK_EQ(selection.frame, 5);
}

TEST_CASE("PieceSpriteSelectorTest - SelectsRestAnimationStates") {
    const kfc::PieceSpriteSelector selector;

    kfc::PieceSpriteContext short_rest{};
    short_rest.resting = true;
    short_rest.rest_kind = kfc::RestKind::Short;
    short_rest.progress = 0.0f;
    CHECK(selector.select(short_rest).state == "short_rest");

    kfc::PieceSpriteContext long_rest{};
    long_rest.resting = true;
    long_rest.rest_kind = kfc::RestKind::Long;
    long_rest.progress = -0.25f;
    CHECK(selector.select(long_rest).state == "long_rest");
    CHECK_EQ(selector.select(long_rest).frame, 1);
}
