#include "database/player_repository.h"
#include "database/sqlite_database.h"
#include "server/authentication_service.h"

#include "server/authentication_service.h"

#include <doctest/doctest.h>

#include <sqlite3.h>

namespace {

struct AuthFixture {
    kfc::SqliteDatabase db{":memory:"};
    kfc::PlayerRepository repo;
    kfc::AuthenticationService auth;

    AuthFixture() : repo(db), auth(repo) {
        REQUIRE(db.open());
        REQUIRE(db.initialize_schema());
    }
};

}  // namespace

TEST_CASE("AuthenticationServiceTest - LoadsExistingUser") {
    AuthFixture fixture;
    REQUIRE(fixture.repo.create_player("alice", 1100).has_value());

    const kfc::AuthenticationResult result = fixture.auth.authenticate("alice", "secret");
    REQUIRE(result.success);
    REQUIRE(result.player.has_value());
    CHECK_EQ(result.player->username(), "alice");
    CHECK_EQ(result.player->rating(), 1100);
}

TEST_CASE("AuthenticationServiceTest - CreatesNewUser") {
    AuthFixture fixture;

    const kfc::AuthenticationResult result = fixture.auth.authenticate("new_user", "pass");
    REQUIRE(result.success);
    REQUIRE(result.player.has_value());
    CHECK_EQ(result.player->username(), "new_user");
    CHECK_EQ(result.player->rating(), 1000);

    const auto stored = fixture.repo.find_by_username("new_user");
    REQUIRE(stored.has_value());
    CHECK_EQ(stored->rating(), 1000);
}

TEST_CASE("AuthenticationServiceTest - RejectsEmptyUsername") {
    AuthFixture fixture;

    const kfc::AuthenticationResult result = fixture.auth.authenticate("", "pass");
    CHECK_FALSE(result.success);
    CHECK_EQ(result.failure_reason, "invalid_username");
}

TEST_CASE("AuthenticationServiceTest - ReportsCreateFailure") {
    const char* path = "kfc_readonly_auth_test.db";
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

    kfc::AuthenticationService auth{repo};
    const kfc::AuthenticationResult result = auth.authenticate("cannot_create", "pass");
    CHECK_FALSE(result.success);
    CHECK_EQ(result.failure_reason, "create_failed");

    std::remove(path);
}
