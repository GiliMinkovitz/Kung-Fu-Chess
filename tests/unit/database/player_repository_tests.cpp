#include "database/player_repository.h"
#include "database/sqlite_database.h"

#include <doctest/doctest.h>

#include <fstream>

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

TEST_CASE("PlayerRepositoryTest - FindsExistingPlayerById") {
    auto repo = make_repository();

    const auto created = repo.create_player("by_id_user", 1200);
    REQUIRE(created.has_value());

    const auto found = repo.find_by_id(created->id());
    REQUIRE(found.has_value());
    CHECK_EQ(found->id(), created->id());
    CHECK_EQ(found->username(), "by_id_user");
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

TEST_CASE("PlayerRepositoryTest - StoresAndLoadsPasswordHash") {
    auto repo = make_repository();
    const std::string hash = "pbkdf2_sha256$10000$abc$def";

    const auto created = repo.create_player("secure", 1000, hash);
    REQUIRE(created.has_value());

    const auto credentials = repo.find_credentials_by_username("secure");
    REQUIRE(credentials.has_value());
    CHECK_EQ(credentials->password_hash, hash);
}

TEST_CASE("PlayerRepositoryTest - ReturnsNulloptWithoutDatabaseConnection") {
    kfc::SqliteDatabase db(":memory:");
    kfc::PlayerRepository repo{db};

    CHECK_FALSE(repo.find_by_username("missing").has_value());
    CHECK_FALSE(repo.create_player("new_user").has_value());
    CHECK_FALSE(repo.update_rating(1, 1000));
}

TEST_CASE("PlayerRepositoryTest - RejectsDuplicateUsername") {
    auto repo = make_repository();

    REQUIRE(repo.create_player("duplicate_user", 1000).has_value());
    CHECK_FALSE(repo.create_player("duplicate_user", 1100).has_value());
}

TEST_CASE("PlayerRepositoryTest - HandlesCorruptDatabaseFile") {
    const char* path = "kfc_corrupt_player_repo_test.db";
    {
        std::ofstream out(path, std::ios::binary);
        out << "not-a-sqlite-database";
    }

    kfc::SqliteDatabase db(path);
    REQUIRE(db.open());
    kfc::PlayerRepository repo{db};

    CHECK_FALSE(repo.find_by_username("alice").has_value());
    CHECK_FALSE(repo.create_player("alice", 1000).has_value());
    CHECK_FALSE(repo.update_rating(1, 1000));
    std::remove(path);
}

TEST_CASE("PlayerRepositoryTest - RejectsWriteWhenDatabaseIsReadOnly") {
    const char* path = "kfc_readonly_player_repo_test.db";
    {
        kfc::SqliteDatabase setup(path);
        REQUIRE(setup.open());
        REQUIRE(setup.initialize_schema());
    }

    kfc::SqliteDatabase db(path);
    REQUIRE(db.open());
    kfc::PlayerRepository repo{db};

    sqlite3* connection = db.connection();
    REQUIRE(connection != nullptr);
    REQUIRE(sqlite3_exec(connection, "PRAGMA query_only = ON;", nullptr, nullptr, nullptr) ==
            SQLITE_OK);

    CHECK_FALSE(repo.create_player("readonly_user", 1000).has_value());
    CHECK_FALSE(repo.update_rating(1, 900));
    std::remove(path);
}
