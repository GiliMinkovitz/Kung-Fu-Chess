#include "database/player_repository.h"
#include "database/sqlite_database.h"

#include <doctest/doctest.h>

namespace {

kfc::PlayerRepository make_repository() {
    static kfc::SqliteDatabase db(":memory:");
    static bool initialized = false;
    if (!initialized) {
        REQUIRE(db.open());
        REQUIRE(db.initialize_schema());
        initialized = true;
    }
    return kfc::PlayerRepository{db};
}

}  // namespace

TEST_CASE("PlayerRepositoryTest - CreatesPlayerWithDefaultRating") {
    auto repo = make_repository();

    const auto player = repo.create_player("alice");
    REQUIRE(player.has_value());
    CHECK_EQ(player->username(), "alice");
    CHECK_EQ(player->rating(), 1000);
    CHECK(player->id() > 0);
}

TEST_CASE("PlayerRepositoryTest - FindsExistingPlayer") {
    auto repo = make_repository();

    const auto created = repo.create_player("bob", 1200);
    REQUIRE(created.has_value());

    const auto found = repo.find_by_username("bob");
    REQUIRE(found.has_value());
    CHECK_EQ(found->id(), created->id());
    CHECK_EQ(found->username(), "bob");
    CHECK_EQ(found->rating(), 1200);
}

TEST_CASE("PlayerRepositoryTest - ReturnsNulloptForMissingPlayer") {
    auto repo = make_repository();

    CHECK_FALSE(repo.find_by_username("missing").has_value());
}

TEST_CASE("PlayerRepositoryTest - UpdatesPlayerRating") {
    auto repo = make_repository();

    const auto created = repo.create_player("carol", 1000);
    REQUIRE(created.has_value());

    CHECK(repo.update_rating(created->id(), 1350));

    const auto found = repo.find_by_username("carol");
    REQUIRE(found.has_value());
    CHECK_EQ(found->rating(), 1350);
}

TEST_CASE("PlayerRepositoryTest - LoadOrCreateFlow") {
    auto repo = make_repository();

    CHECK_FALSE(repo.find_by_username("Player1").has_value());

    const auto created = repo.create_player("Player1", 1000);
    REQUIRE(created.has_value());

    const auto loaded = repo.find_by_username("Player1");
    REQUIRE(loaded.has_value());
    CHECK_EQ(loaded->id(), created->id());
    CHECK_EQ(loaded->rating(), 1000);
}
