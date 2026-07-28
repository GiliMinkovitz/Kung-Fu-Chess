#include "app/database_config.h"
#include "app/server_infrastructure.h"
#include "database/postgres_connection.h"

#include <doctest/doctest.h>

#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <string>

TEST_CASE("DatabaseConfigTest - DefaultsToSQLiteBackend") {
    const kfc::app::DatabaseConfig config;
    CHECK(config.backend == kfc::app::DatabaseBackend::SQLite);
    CHECK_EQ(config.path, "kfc.db");
    CHECK_EQ(config.host, "localhost");
    CHECK_EQ(config.port, 5432);
    CHECK_EQ(config.database, "kfc");
    CHECK(config.username.empty());
    CHECK(config.password.empty());
}

TEST_CASE("PostgresConnectionTest - DoesNotExposeSqliteConnection") {
    kfc::PostgresConnection connection{kfc::PostgresConnection::Settings{
        "127.0.0.1",
        1,
        "missing_database",
        "missing_user",
        "missing_password",
    }};

    CHECK(connection.connection() == nullptr);
    CHECK(connection.native_connection() == nullptr);
}

TEST_CASE("PostgresConnectionTest - OpenFailsWithoutServer") {
    kfc::PostgresConnection connection{kfc::PostgresConnection::Settings{
        "127.0.0.1",
        1,
        "missing_database",
        "missing_user",
        "missing_password",
    }};

    CHECK_FALSE(connection.open());
    CHECK(connection.connection() == nullptr);
#if KFC_HAS_LIBPQ
    CHECK(connection.native_connection() != nullptr);
#endif
    CHECK_FALSE(connection.initialize_schema());
}

TEST_CASE("PostgresConnectionTest - OpenIsIdempotentWhenConnectionFailed") {
    kfc::PostgresConnection connection{kfc::PostgresConnection::Settings{
        "127.0.0.1",
        1,
        "missing_database",
    }};

    CHECK_FALSE(connection.open());
    CHECK_FALSE(connection.open());
}

TEST_CASE("PostgresConnectionTest - InitializeSchemaRequiresOpenConnection") {
    kfc::PostgresConnection connection{kfc::PostgresConnection::Settings{
        "127.0.0.1",
        5432,
        "kfc",
        "kfc",
        "kfc",
    }};

    CHECK_FALSE(connection.initialize_schema());
}

TEST_CASE("ServerInfrastructureTest - PostgreSQLBackendFailsWithoutServer") {
    kfc::app::DatabaseConfig config;
    config.backend = kfc::app::DatabaseBackend::PostgreSQL;

    CHECK_THROWS_AS((void)kfc::app::ServerInfrastructure{config}, std::runtime_error);
}

#if KFC_HAS_LIBPQ
#include <libpq-fe.h>

namespace {

std::optional<kfc::PostgresConnection::Settings> live_postgres_settings_from_environment() {
    const char* host = std::getenv("KFC_DB_HOST");
    if (host == nullptr || host[0] == '\0') {
        return std::nullopt;
    }

    kfc::PostgresConnection::Settings settings;
    settings.host = host;
    settings.port = 5432;
    settings.database = "kfc";
    settings.username = "kfc";
    settings.password = "kfc";

    if (const char* port = std::getenv("KFC_DB_PORT"); port != nullptr && port[0] != '\0') {
        settings.port = std::atoi(port);
    }
    if (const char* database = std::getenv("KFC_DB_NAME"); database != nullptr && database[0] != '\0') {
        settings.database = database;
    }
    if (const char* username = std::getenv("KFC_DB_USER"); username != nullptr && username[0] != '\0') {
        settings.username = username;
    }
    if (const char* password = std::getenv("KFC_DB_PASSWORD"); password != nullptr) {
        settings.password = password;
    }

    return settings;
}

bool table_exists(PGconn* connection, const std::string& table_name) {
    if (connection == nullptr || PQstatus(connection) != CONNECTION_OK) {
        return false;
    }

    const char* params[] = {table_name.c_str()};
    PGresult* result = PQexecParams(
        connection,
        "SELECT 1 FROM information_schema.tables "
        "WHERE table_schema = 'public' AND table_name = $1 LIMIT 1;",
        1,
        nullptr,
        params,
        nullptr,
        nullptr,
        0);
    if (result == nullptr) {
        return false;
    }

    const bool exists =
        PQresultStatus(result) == PGRES_TUPLES_OK && PQntuples(result) > 0;
    PQclear(result);
    return exists;
}

}  // namespace

TEST_CASE("PostgresConnectionTest - InitializesSchemaWithLiveDatabase") {
    const auto settings = live_postgres_settings_from_environment();
    if (!settings.has_value()) {
        WARN("Skipping live PostgreSQL schema test; set KFC_DB_HOST to run against a real instance");
        return;
    }

    kfc::PostgresConnection connection{*settings};
    REQUIRE(connection.open());
    CHECK(connection.initialize_schema());

    PGconn* native = connection.native_connection();
    REQUIRE(native != nullptr);
    CHECK(table_exists(native, "players"));
    CHECK(table_exists(native, "games"));
}

TEST_CASE("PostgresConnectionTest - ReinitializingSchemaIsIdempotent") {
    const auto settings = live_postgres_settings_from_environment();
    if (!settings.has_value()) {
        WARN("Skipping live PostgreSQL schema test; set KFC_DB_HOST to run against a real instance");
        return;
    }

    kfc::PostgresConnection connection{*settings};
    REQUIRE(connection.open());
    CHECK(connection.initialize_schema());
    CHECK(connection.initialize_schema());
}
#endif
