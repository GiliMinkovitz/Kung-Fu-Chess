#include "server/database/sqlite_user_repository.h"
#include "database/sqlite_database.h"
#include "server/authentication_service.h"
#include "server/password_hasher.h"

#include <doctest/doctest.h>

#include <sqlite3.h>

namespace {

struct AuthFixture {
    kfc::SqliteDatabase db{":memory:"};
    kfc::SqliteUserRepository repo;
    kfc::AuthenticationService auth;

    AuthFixture() : repo(db), auth(repo) {
        REQUIRE(db.open());
        REQUIRE(db.initialize_schema());
    }
};

}  // namespace

TEST_CASE("AuthenticationServiceTest - RegistersNewUser") {
    AuthFixture fixture;

    const kfc::AuthenticationResult result = fixture.auth.authenticate("new_user", "secret");
    REQUIRE(result.success);
    REQUIRE(result.player.has_value());
    CHECK_EQ(result.player->username(), "new_user");
    CHECK_EQ(result.player->rating(), 1000);

    const auto credentials = fixture.repo.find_credentials_by_username("new_user");
    REQUIRE(credentials.has_value());
    CHECK_FALSE(credentials->password_hash.empty());
    CHECK(credentials->password_hash.find("pbkdf2_sha256$") == 0);
    CHECK(credentials->password_hash.find("secret") == std::string::npos);
}

TEST_CASE("AuthenticationServiceTest - LogsInExistingUserWithCorrectPassword") {
    AuthFixture fixture;
    REQUIRE(fixture.auth.authenticate("alice", "secret").success);

    const kfc::AuthenticationResult result = fixture.auth.authenticate("alice", "secret");
    REQUIRE(result.success);
    REQUIRE(result.player.has_value());
    CHECK_EQ(result.player->username(), "alice");
    CHECK_EQ(result.player->rating(), 1000);
}

TEST_CASE("AuthenticationServiceTest - RejectsWrongPassword") {
    AuthFixture fixture;
    REQUIRE(fixture.auth.authenticate("alice", "secret").success);

    const kfc::AuthenticationResult result = fixture.auth.authenticate("alice", "wrong");
    CHECK_FALSE(result.success);
    CHECK_EQ(result.failure_reason, "invalid_password");
}

TEST_CASE("AuthenticationServiceTest - RejectsMissingPassword") {
    AuthFixture fixture;

    const kfc::AuthenticationResult result = fixture.auth.authenticate("alice", "");
    CHECK_FALSE(result.success);
    CHECK_EQ(result.failure_reason, "missing_password");
}

TEST_CASE("AuthenticationServiceTest - RejectsEmptyUsername") {
    AuthFixture fixture;

    const kfc::AuthenticationResult result = fixture.auth.authenticate("", "pass");
    CHECK_FALSE(result.success);
    CHECK_EQ(result.failure_reason, "invalid_username");
}

TEST_CASE("AuthenticationServiceTest - RejectsLegacyUserWithoutPasswordHash") {
    AuthFixture fixture;
    REQUIRE(fixture.repo.create_user("legacy_user") != 0);

    const kfc::AuthenticationResult result = fixture.auth.authenticate("legacy_user", "secret");
    CHECK_FALSE(result.success);
    CHECK_EQ(result.failure_reason, "invalid_password");
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
    kfc::SqliteUserRepository repo{db};
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

TEST_CASE("AuthenticationServiceTest - StoresHashedPasswordNotPlaintext") {
    AuthFixture fixture;
    REQUIRE(fixture.auth.authenticate("secure_user", "my_password").success);

    const auto credentials = fixture.repo.find_credentials_by_username("secure_user");
    REQUIRE(credentials.has_value());
    CHECK_NE(credentials->password_hash, "my_password");
    CHECK(kfc::PasswordHasher::verify_password("my_password", credentials->password_hash));
}
