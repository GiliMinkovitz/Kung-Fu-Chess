#include "database/postgres_game_repository.h"
#include "database/postgres_connection.h"
#include "server/authentication_service.h"
#include "server/database/postgres_user_repository.h"

#include <doctest/doctest.h>

#include <atomic>
#include <cstdlib>
#include <optional>
#include <string>

#if KFC_HAS_LIBPQ

namespace {

std::atomic<int> g_username_counter{0};

std::string unique_username(const char* prefix) {
    return std::string(prefix) + "_" + std::to_string(++g_username_counter);
}

std::optional<kfc::PostgresConnection::Settings> live_postgres_settings_from_environment() {
    const char* host = std::getenv("KFC_POSTGRES_HOST");
    if (host == nullptr || host[0] == '\0') {
        host = std::getenv("KFC_DB_HOST");
    }
    if (host == nullptr || host[0] == '\0') {
        return std::nullopt;
    }

    kfc::PostgresConnection::Settings settings;
    settings.host = host;
    settings.port = 5432;
    settings.database = "kfc";
    settings.username = "kfc";
    settings.password = "kfc";

    if (const char* port = std::getenv("KFC_POSTGRES_PORT"); port != nullptr && port[0] != '\0') {
        settings.port = std::atoi(port);
    } else if (const char* port = std::getenv("KFC_DB_PORT"); port != nullptr && port[0] != '\0') {
        settings.port = std::atoi(port);
    }
    if (const char* database = std::getenv("KFC_POSTGRES_DB"); database != nullptr && database[0] != '\0') {
        settings.database = database;
    } else if (const char* database = std::getenv("KFC_DB_NAME"); database != nullptr && database[0] != '\0') {
        settings.database = database;
    }
    if (const char* username = std::getenv("KFC_POSTGRES_USER"); username != nullptr && username[0] != '\0') {
        settings.username = username;
    } else if (const char* username = std::getenv("KFC_DB_USER"); username != nullptr && username[0] != '\0') {
        settings.username = username;
    }
    if (const char* password = std::getenv("KFC_POSTGRES_PASSWORD"); password != nullptr) {
        settings.password = password;
    } else if (const char* password = std::getenv("KFC_DB_PASSWORD"); password != nullptr) {
        settings.password = password;
    }

    return settings;
}

struct LivePostgresFixture {
    kfc::PostgresConnection connection;
    kfc::PostgresUserRepository user_repository;
    kfc::PostgresGameRepository game_repository;
    kfc::AuthenticationService authentication;

    explicit LivePostgresFixture(kfc::PostgresConnection::Settings settings)
        : connection(std::move(settings)),
          user_repository(connection),
          game_repository(connection),
          authentication(user_repository) {
        REQUIRE(connection.open());
        REQUIRE(connection.initialize_schema());
    }
};

}  // namespace

TEST_CASE("PostgresRepositoryIntegrationTest - CreatesUserAndAuthenticates") {
    const auto settings = live_postgres_settings_from_environment();
    if (!settings.has_value()) {
        WARN("Skipping live PostgreSQL repository test; set KFC_POSTGRES_HOST or KFC_DB_HOST");
        return;
    }

    LivePostgresFixture fixture{*settings};
    const std::string username = unique_username("auth_user");

    const kfc::AuthenticationResult register_result = fixture.authentication.authenticate(username, "secret");
    REQUIRE(register_result.success);
    REQUIRE(register_result.player.has_value());
    CHECK_EQ(register_result.player->username(), username);
    CHECK_EQ(register_result.player->rating(), 1000);

    const kfc::AuthenticationResult login_result = fixture.authentication.authenticate(username, "secret");
    REQUIRE(login_result.success);
    CHECK_EQ(login_result.player->username(), username);

    const kfc::AuthenticationResult bad_password = fixture.authentication.authenticate(username, "wrong");
    CHECK_FALSE(bad_password.success);
}

TEST_CASE("PostgresRepositoryIntegrationTest - CreatesAndFinishesGame") {
    const auto settings = live_postgres_settings_from_environment();
    if (!settings.has_value()) {
        WARN("Skipping live PostgreSQL repository test; set KFC_POSTGRES_HOST or KFC_DB_HOST");
        return;
    }

    LivePostgresFixture fixture{*settings};
    const std::string white_name = unique_username("white");
    const std::string black_name = unique_username("black");

    const auto white = fixture.user_repository.create_user_with_password(white_name, 1000, "hash");
    const auto black = fixture.user_repository.create_user_with_password(black_name, 1000, "hash");
    REQUIRE(white.has_value());
    REQUIRE(black.has_value());

    const std::optional<int> game_id =
        fixture.game_repository.create_game(white->id(), black->id());
    REQUIRE(game_id.has_value());
    CHECK(*game_id > 0);

    CHECK(fixture.game_repository.finish_game(*game_id, white->id()));
    CHECK_FALSE(fixture.game_repository.finish_game_without_winner(*game_id));
}

TEST_CASE("PostgresRepositoryIntegrationTest - UpdatesRating") {
    const auto settings = live_postgres_settings_from_environment();
    if (!settings.has_value()) {
        WARN("Skipping live PostgreSQL repository test; set KFC_POSTGRES_HOST or KFC_DB_HOST");
        return;
    }

    LivePostgresFixture fixture{*settings};
    const std::string username = unique_username("rated_user");

    const auto created = fixture.user_repository.create_user_with_password(username, 1000, "hash");
    REQUIRE(created.has_value());
    CHECK_EQ(created->rating(), 1000);

    REQUIRE(fixture.user_repository.update_rating(created->id(), 1250));

    const auto profile = fixture.user_repository.find_profile_by_id(created->id());
    REQUIRE(profile.has_value());
    CHECK_EQ(profile->rating(), 1250);
}

#endif
