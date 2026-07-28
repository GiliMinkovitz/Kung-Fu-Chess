#include "app/database_config.h"
#include "app/server_health.h"
#include "app/server_infrastructure.h"
#include "database/i_database_connection.h"

#include <doctest/doctest.h>

#include <cstdlib>
#include <optional>
#include <string>

namespace {

class DisconnectedDatabase final : public kfc::IDatabaseConnection {
public:
    [[nodiscard]] bool is_connected() const override { return false; }
    sqlite3* connection() override { return nullptr; }
};

#if KFC_HAS_LIBPQ
std::optional<kfc::app::DatabaseConfig> live_postgres_database_config() {
    const char* host = std::getenv("KFC_DB_HOST");
    if (host == nullptr || host[0] == '\0') {
        return std::nullopt;
    }

    kfc::app::DatabaseConfig config;
    config.backend = kfc::app::DatabaseBackend::PostgreSQL;
    config.host = host;
    config.port = 5432;
    config.database = "kfc";
    config.username = "kfc";
    config.password = "kfc";

    if (const char* port = std::getenv("KFC_DB_PORT"); port != nullptr && port[0] != '\0') {
        config.port = std::atoi(port);
    }
    if (const char* database = std::getenv("KFC_DB_NAME"); database != nullptr && database[0] != '\0') {
        config.database = database;
    }
    if (const char* username = std::getenv("KFC_DB_USER"); username != nullptr && username[0] != '\0') {
        config.username = username;
    }
    if (const char* password = std::getenv("KFC_DB_PASSWORD"); password != nullptr) {
        config.password = password;
    }

    return config;
}
#endif

}  // namespace

TEST_CASE("ServerHealthTest - SQLiteDefaultHealthStatus") {
    kfc::app::DatabaseConfig config;
    config.path = ":memory:";
    kfc::app::ServerInfrastructure infrastructure{config};

    const kfc::app::HealthStatus running = infrastructure.get_health_status(true);
    CHECK(running.server_running);
    CHECK(running.database_connected);
    CHECK_EQ(running.database_backend, "SQLite");

    const kfc::app::HealthStatus stopped = infrastructure.get_health_status(false);
    CHECK_FALSE(stopped.server_running);
    CHECK(stopped.database_connected);
    CHECK_EQ(stopped.database_backend, "SQLite");
}

TEST_CASE("ServerHealthTest - UnavailableDatabaseConnection") {
    DisconnectedDatabase database;
    const kfc::app::HealthStatus status =
        kfc::app::make_health_status(true, kfc::app::DatabaseBackend::SQLite, database);

    CHECK(status.server_running);
    CHECK_FALSE(status.database_connected);
    CHECK_EQ(status.database_backend, "SQLite");
}

#if KFC_HAS_LIBPQ
TEST_CASE("ServerHealthTest - PostgreSQLConfigurationHealthStatus") {
    const auto config = live_postgres_database_config();
    if (!config.has_value()) {
        WARN("Skipping live PostgreSQL health test; set KFC_DB_HOST to run against a real instance");
        return;
    }

    kfc::app::ServerInfrastructure infrastructure{*config};
    const kfc::app::HealthStatus status = infrastructure.get_health_status(true);

    CHECK(status.server_running);
    CHECK(status.database_connected);
    CHECK_EQ(status.database_backend, "PostgreSQL");
}
#endif
