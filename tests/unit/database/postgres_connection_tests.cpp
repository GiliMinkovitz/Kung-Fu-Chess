#include "app/database_config.h"
#include "app/server_infrastructure.h"
#include "database/postgres_connection.h"

#include <doctest/doctest.h>

#include <stdexcept>

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

TEST_CASE("ServerInfrastructureTest - RejectsPostgreSQLBackendUntilRepositoriesExist") {
    kfc::app::DatabaseConfig config;
    config.backend = kfc::app::DatabaseBackend::PostgreSQL;

    CHECK_THROWS_AS((kfc::app::ServerInfrastructure{config}), std::runtime_error);
}
