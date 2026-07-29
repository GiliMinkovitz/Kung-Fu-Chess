#include "app/config_loader.h"
#include "app/database_config.h"
#include "app/logging_config.h"
#include "app/matchmaking_config.h"
#include "app/server_config.h"

#include <doctest/doctest.h>

#include <chrono>
#include <cstdlib>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

#ifdef _WIN32
void set_environment(const char* name, const char* value) {
    _putenv_s(name, value);
}

void clear_environment(const char* name) {
    _putenv_s(name, "");
}
#else
void set_environment(const char* name, const char* value) {
    setenv(name, value, 1);
}

void clear_environment(const char* name) {
    unsetenv(name);
}
#endif

class ScopedEnvironment {
public:
    explicit ScopedEnvironment(std::initializer_list<std::pair<const char*, std::optional<const char*>>> overrides) {
        for (const auto& [name, value] : overrides) {
            save(name);
            if (value.has_value()) {
                set_environment(name, *value);
            } else {
                clear_environment(name);
            }
        }
    }

    ~ScopedEnvironment() {
        for (auto it = saved_.rbegin(); it != saved_.rend(); ++it) {
            if (it->second.has_value()) {
                set_environment(it->first.c_str(), it->second->c_str());
            } else {
                clear_environment(it->first.c_str());
            }
        }
    }

    ScopedEnvironment(const ScopedEnvironment&) = delete;
    ScopedEnvironment& operator=(const ScopedEnvironment&) = delete;

private:
    void save(const char* name) {
        if (const char* value = std::getenv(name)) {
            saved_.emplace_back(name, std::string(value));
        } else {
            saved_.emplace_back(name, std::nullopt);
        }
    }

    std::vector<std::pair<std::string, std::optional<std::string>>> saved_;
};

void check_default_server_config(const kfc::app::ServerConfig& server) {
    CHECK_EQ(server.port, 8765);
    CHECK_EQ(server.health_port, 8080);
    CHECK_EQ(server.bind_address, "127.0.0.1");
    CHECK_EQ(server.server_id, "local");
    CHECK_EQ(server.region, "local");
    CHECK(server.endpoint.empty());
    CHECK_EQ(server.max_clients, kfc::app::ServerConfig::kDefaultMaxClients);
}

void check_default_database_config(const kfc::app::DatabaseConfig& database) {
    CHECK(database.backend == kfc::app::DatabaseBackend::SQLite);
    CHECK_EQ(database.path, "kfc.db");
    CHECK_EQ(database.host, "localhost");
    CHECK_EQ(database.port, 5432);
    CHECK_EQ(database.database, "kfc");
    CHECK(database.username.empty());
    CHECK(database.password.empty());
}

constexpr std::pair<const char*, std::optional<const char*>> kClearDbBackend{"KFC_DB_BACKEND", std::nullopt};
constexpr std::pair<const char*, std::optional<const char*>> kClearDbPath{"KFC_DB_PATH", std::nullopt};
constexpr std::pair<const char*, std::optional<const char*>> kClearDbHost{"KFC_DB_HOST", std::nullopt};
constexpr std::pair<const char*, std::optional<const char*>> kClearDbPort{"KFC_DB_PORT", std::nullopt};
constexpr std::pair<const char*, std::optional<const char*>> kClearDbName{"KFC_DB_NAME", std::nullopt};
constexpr std::pair<const char*, std::optional<const char*>> kClearDbUser{"KFC_DB_USER", std::nullopt};
constexpr std::pair<const char*, std::optional<const char*>> kClearDbPassword{"KFC_DB_PASSWORD", std::nullopt};
constexpr std::pair<const char*, std::optional<const char*>> kClearMatchMaxRatingDiff{
    "KFC_MATCH_MAX_RATING_DIFF", std::nullopt};
constexpr std::pair<const char*, std::optional<const char*>> kClearMatchQueueTimeout{
    "KFC_MATCH_QUEUE_TIMEOUT_SEC", std::nullopt};
constexpr std::pair<const char*, std::optional<const char*>> kClearDiagnostics{"KFC_DIAGNOSTICS", std::nullopt};

void check_default_matchmaking_config(const kfc::app::MatchmakingConfig& matchmaking) {
    CHECK_EQ(matchmaking.max_rating_difference, 100);
    CHECK_EQ(matchmaking.queue_timeout, std::chrono::seconds(60));
}

void check_default_logging_config(const kfc::app::LoggingConfig& logging) {
    CHECK(logging.diagnostics_enabled);
}

}  // namespace

TEST_CASE("ConfigLoaderTest - DefaultConfigurationWithoutEnvironment") {
    const ScopedEnvironment env{
        {"KFC_PORT", std::nullopt},
        {"KFC_BIND_ADDRESS", std::nullopt},
        {"KFC_MAX_CLIENTS", std::nullopt},
        {"KFC_HEALTH_PORT", std::nullopt},
        {"KFC_SERVER_ID", std::nullopt},
        {"KFC_REGION", std::nullopt},
        {"KFC_GAME_ENDPOINT", std::nullopt},
        kClearDbBackend,
        kClearDbPath,
        kClearDbHost,
        kClearDbPort,
        kClearDbName,
        kClearDbUser,
        kClearDbPassword,
        kClearMatchMaxRatingDiff,
        kClearMatchQueueTimeout,
        kClearDiagnostics,
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    check_default_server_config(config.server);
    check_default_database_config(config.database);
    check_default_matchmaking_config(config.matchmaking);
    check_default_logging_config(config.logging);
}

TEST_CASE("ConfigLoaderTest - DefaultMatchmakingConfigurationWithoutEnvironment") {
    const ScopedEnvironment env{
        kClearMatchMaxRatingDiff,
        kClearMatchQueueTimeout,
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    check_default_matchmaking_config(config.matchmaking);
}

TEST_CASE("ConfigLoaderTest - OverridesMatchmakingMaxRatingDifference") {
    const ScopedEnvironment env{
        {"KFC_MATCH_MAX_RATING_DIFF", "250"},
        kClearMatchQueueTimeout,
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK_EQ(config.matchmaking.max_rating_difference, 250);
    CHECK_EQ(config.matchmaking.queue_timeout, std::chrono::seconds(60));
}

TEST_CASE("ConfigLoaderTest - OverridesMatchmakingQueueTimeout") {
    const ScopedEnvironment env{
        kClearMatchMaxRatingDiff,
        {"KFC_MATCH_QUEUE_TIMEOUT_SEC", "120"},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK_EQ(config.matchmaking.max_rating_difference, 100);
    CHECK_EQ(config.matchmaking.queue_timeout, std::chrono::seconds(120));
}

TEST_CASE("ConfigLoaderTest - AppliesMatchmakingOverridesTogether") {
    const ScopedEnvironment env{
        {"KFC_MATCH_MAX_RATING_DIFF", "250"},
        {"KFC_MATCH_QUEUE_TIMEOUT_SEC", "120"},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK_EQ(config.matchmaking.max_rating_difference, 250);
    CHECK_EQ(config.matchmaking.queue_timeout, std::chrono::seconds(120));
}

TEST_CASE("ConfigLoaderTest - IgnoresInvalidMatchmakingValues") {
    const ScopedEnvironment env{
        {"KFC_MATCH_MAX_RATING_DIFF", "abc"},
        {"KFC_MATCH_QUEUE_TIMEOUT_SEC", "not-a-number"},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    check_default_matchmaking_config(config.matchmaking);
}

TEST_CASE("ConfigLoaderTest - IgnoresZeroMatchmakingValues") {
    const ScopedEnvironment env{
        {"KFC_MATCH_MAX_RATING_DIFF", "0"},
        {"KFC_MATCH_QUEUE_TIMEOUT_SEC", "0"},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    check_default_matchmaking_config(config.matchmaking);
}

TEST_CASE("ConfigLoaderTest - IgnoresNegativeMatchmakingValues") {
    const ScopedEnvironment env{
        {"KFC_MATCH_MAX_RATING_DIFF", "-50"},
        {"KFC_MATCH_QUEUE_TIMEOUT_SEC", "-10"},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    check_default_matchmaking_config(config.matchmaking);
}

TEST_CASE("ConfigLoaderTest - MissingMatchmakingVariablesKeepDefaults") {
    const ScopedEnvironment env{
        kClearMatchMaxRatingDiff,
        kClearMatchQueueTimeout,
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    check_default_matchmaking_config(config.matchmaking);
}

TEST_CASE("ConfigLoaderTest - OverridesDiagnosticsEnabled") {
    SUBCASE("True") {
        const ScopedEnvironment env{{"KFC_DIAGNOSTICS", "true"}};
        const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
        CHECK(config.logging.diagnostics_enabled);
    }

    SUBCASE("False") {
        const ScopedEnvironment env{{"KFC_DIAGNOSTICS", "false"}};
        const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
        CHECK_FALSE(config.logging.diagnostics_enabled);
    }

    SUBCASE("UppercaseTrue") {
        const ScopedEnvironment env{{"KFC_DIAGNOSTICS", "TRUE"}};
        const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
        CHECK(config.logging.diagnostics_enabled);
    }

    SUBCASE("UppercaseFalse") {
        const ScopedEnvironment env{{"KFC_DIAGNOSTICS", "FALSE"}};
        const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
        CHECK_FALSE(config.logging.diagnostics_enabled);
    }

    SUBCASE("One") {
        const ScopedEnvironment env{{"KFC_DIAGNOSTICS", "1"}};
        const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
        CHECK(config.logging.diagnostics_enabled);
    }

    SUBCASE("Zero") {
        const ScopedEnvironment env{{"KFC_DIAGNOSTICS", "0"}};
        const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
        CHECK_FALSE(config.logging.diagnostics_enabled);
    }

    SUBCASE("Yes") {
        const ScopedEnvironment env{{"KFC_DIAGNOSTICS", "yes"}};
        const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
        CHECK(config.logging.diagnostics_enabled);
    }

    SUBCASE("No") {
        const ScopedEnvironment env{{"KFC_DIAGNOSTICS", "no"}};
        const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
        CHECK_FALSE(config.logging.diagnostics_enabled);
    }

    SUBCASE("On") {
        const ScopedEnvironment env{{"KFC_DIAGNOSTICS", "on"}};
        const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
        CHECK(config.logging.diagnostics_enabled);
    }

    SUBCASE("Off") {
        const ScopedEnvironment env{{"KFC_DIAGNOSTICS", "off"}};
        const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
        CHECK_FALSE(config.logging.diagnostics_enabled);
    }
}

TEST_CASE("ConfigLoaderTest - IgnoresInvalidDiagnosticsValues") {
    SUBCASE("Banana") {
        const ScopedEnvironment env{{"KFC_DIAGNOSTICS", "banana"}};
        const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
        check_default_logging_config(config.logging);
    }

    SUBCASE("Abc") {
        const ScopedEnvironment env{{"KFC_DIAGNOSTICS", "abc"}};
        const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
        check_default_logging_config(config.logging);
    }

    SUBCASE("QuestionMarks") {
        const ScopedEnvironment env{{"KFC_DIAGNOSTICS", "???"}};
        const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
        check_default_logging_config(config.logging);
    }

    SUBCASE("Empty") {
        const ScopedEnvironment env{{"KFC_DIAGNOSTICS", ""}};
        const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
        check_default_logging_config(config.logging);
    }
}

TEST_CASE("ConfigLoaderTest - DefaultDatabaseBackendWithoutEnvironment") {
    const ScopedEnvironment env{
        kClearDbBackend,
        kClearDbPath,
        kClearDbHost,
        kClearDbPort,
        kClearDbName,
        kClearDbUser,
        kClearDbPassword,
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    check_default_database_config(config.database);
}

TEST_CASE("ConfigLoaderTest - SelectsSqliteBackend") {
    const ScopedEnvironment env{
        {"KFC_DB_BACKEND", "SQLITE"},
        kClearDbPath,
        kClearDbHost,
        kClearDbPort,
        kClearDbName,
        kClearDbUser,
        kClearDbPassword,
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK(config.database.backend == kfc::app::DatabaseBackend::SQLite);
}

TEST_CASE("ConfigLoaderTest - SelectsPostgresBackend") {
    const ScopedEnvironment env{
        {"KFC_DB_BACKEND", "Postgres"},
        kClearDbPath,
        kClearDbHost,
        kClearDbPort,
        kClearDbName,
        kClearDbUser,
        kClearDbPassword,
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK(config.database.backend == kfc::app::DatabaseBackend::PostgreSQL);
}

TEST_CASE("ConfigLoaderTest - IgnoresInvalidDatabaseBackend") {
    const ScopedEnvironment env{
        {"KFC_DB_BACKEND", "mysql"},
        kClearDbPath,
        kClearDbHost,
        kClearDbPort,
        kClearDbName,
        kClearDbUser,
        kClearDbPassword,
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK(config.database.backend == kfc::app::DatabaseBackend::SQLite);
}

TEST_CASE("ConfigLoaderTest - OverridesDatabasePath") {
    const ScopedEnvironment env{
        kClearDbBackend,
        {"KFC_DB_PATH", "/tmp/custom.db"},
        kClearDbHost,
        kClearDbPort,
        kClearDbName,
        kClearDbUser,
        kClearDbPassword,
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK_EQ(config.database.path, "/tmp/custom.db");
    CHECK(config.database.backend == kfc::app::DatabaseBackend::SQLite);
}

TEST_CASE("ConfigLoaderTest - OverridesPostgresHost") {
    const ScopedEnvironment env{
        kClearDbBackend,
        kClearDbPath,
        {"KFC_DB_HOST", "db.example.com"},
        kClearDbPort,
        kClearDbName,
        kClearDbUser,
        kClearDbPassword,
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK_EQ(config.database.host, "db.example.com");
}

TEST_CASE("ConfigLoaderTest - OverridesPostgresPort") {
    const ScopedEnvironment env{
        kClearDbBackend,
        kClearDbPath,
        kClearDbHost,
        {"KFC_DB_PORT", "5433"},
        kClearDbName,
        kClearDbUser,
        kClearDbPassword,
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK_EQ(config.database.port, 5433);
}

TEST_CASE("ConfigLoaderTest - OverridesPostgresDatabaseName") {
    const ScopedEnvironment env{
        kClearDbBackend,
        kClearDbPath,
        kClearDbHost,
        kClearDbPort,
        {"KFC_DB_NAME", "production"},
        kClearDbUser,
        kClearDbPassword,
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK_EQ(config.database.database, "production");
}

TEST_CASE("ConfigLoaderTest - OverridesPostgresUsername") {
    const ScopedEnvironment env{
        kClearDbBackend,
        kClearDbPath,
        kClearDbHost,
        kClearDbPort,
        kClearDbName,
        {"KFC_DB_USER", "admin"},
        kClearDbPassword,
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK_EQ(config.database.username, "admin");
}

TEST_CASE("ConfigLoaderTest - OverridesPostgresPassword") {
    const ScopedEnvironment env{
        kClearDbBackend,
        kClearDbPath,
        kClearDbHost,
        kClearDbPort,
        kClearDbName,
        kClearDbUser,
        {"KFC_DB_PASSWORD", "secret"},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK_EQ(config.database.password, "secret");
}

TEST_CASE("ConfigLoaderTest - AppliesFullPostgresConfiguration") {
    const ScopedEnvironment env{
        {"KFC_DB_BACKEND", "postgres"},
        kClearDbPath,
        {"KFC_DB_HOST", "db.example.com"},
        {"KFC_DB_PORT", "5433"},
        {"KFC_DB_NAME", "production"},
        {"KFC_DB_USER", "admin"},
        {"KFC_DB_PASSWORD", "secret"},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK(config.database.backend == kfc::app::DatabaseBackend::PostgreSQL);
    CHECK_EQ(config.database.host, "db.example.com");
    CHECK_EQ(config.database.port, 5433);
    CHECK_EQ(config.database.database, "production");
    CHECK_EQ(config.database.username, "admin");
    CHECK_EQ(config.database.password, "secret");
}

TEST_CASE("ConfigLoaderTest - IgnoresInvalidPostgresPort") {
    SUBCASE("NonNumericPort") {
        const ScopedEnvironment env{
            kClearDbBackend,
            kClearDbPath,
            kClearDbHost,
            {"KFC_DB_PORT", "abc"},
            kClearDbName,
            kClearDbUser,
            kClearDbPassword,
        };

        const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
        CHECK_EQ(config.database.port, 5432);
    }

    SUBCASE("OutOfRangePort") {
        const ScopedEnvironment env{
            kClearDbBackend,
            kClearDbPath,
            kClearDbHost,
            {"KFC_DB_PORT", "70000"},
            kClearDbName,
            kClearDbUser,
            kClearDbPassword,
        };

        const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
        CHECK_EQ(config.database.port, 5432);
    }
}

TEST_CASE("ConfigLoaderTest - MissingDatabaseVariablesKeepDefaults") {
    const ScopedEnvironment env{
        kClearDbBackend,
        kClearDbPath,
        kClearDbHost,
        kClearDbPort,
        kClearDbName,
        kClearDbUser,
        kClearDbPassword,
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    check_default_database_config(config.database);
}

TEST_CASE("ConfigLoaderTest - OverridesPort") {
    const ScopedEnvironment env{
        {"KFC_PORT", "9000"},
        {"KFC_BIND_ADDRESS", std::nullopt},
        {"KFC_MAX_CLIENTS", std::nullopt},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK_EQ(config.server.port, 9000);
    CHECK_EQ(config.server.bind_address, "127.0.0.1");
    CHECK_EQ(config.server.max_clients, kfc::app::ServerConfig::kDefaultMaxClients);
}

TEST_CASE("ConfigLoaderTest - OverridesBindAddress") {
    const ScopedEnvironment env{
        {"KFC_PORT", std::nullopt},
        {"KFC_BIND_ADDRESS", "0.0.0.0"},
        {"KFC_MAX_CLIENTS", std::nullopt},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK_EQ(config.server.port, 8765);
    CHECK_EQ(config.server.bind_address, "0.0.0.0");
    CHECK_EQ(config.server.max_clients, kfc::app::ServerConfig::kDefaultMaxClients);
}

TEST_CASE("ConfigLoaderTest - OverridesMaxClients") {
    const ScopedEnvironment env{
        {"KFC_PORT", std::nullopt},
        {"KFC_BIND_ADDRESS", std::nullopt},
        {"KFC_MAX_CLIENTS", "64"},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK_EQ(config.server.port, 8765);
    CHECK_EQ(config.server.bind_address, "127.0.0.1");
    CHECK_EQ(config.server.max_clients, 64);
}

TEST_CASE("ConfigLoaderTest - AppliesMultipleOverridesTogether") {
    const ScopedEnvironment env{
        {"KFC_PORT", "9000"},
        {"KFC_BIND_ADDRESS", "0.0.0.0"},
        {"KFC_MAX_CLIENTS", "64"},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK_EQ(config.server.port, 9000);
    CHECK_EQ(config.server.bind_address, "0.0.0.0");
    CHECK_EQ(config.server.max_clients, 64);
}

TEST_CASE("ConfigLoaderTest - IgnoresInvalidNumericValues") {
    const ScopedEnvironment env{
        {"KFC_PORT", "abc"},
        {"KFC_BIND_ADDRESS", std::nullopt},
        {"KFC_MAX_CLIENTS", "not-a-number"},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    check_default_server_config(config.server);
}

TEST_CASE("ConfigLoaderTest - IgnoresZeroAndOutOfRangePort") {
    SUBCASE("ZeroPort") {
        const ScopedEnvironment env{
            {"KFC_PORT", "0"},
            {"KFC_BIND_ADDRESS", std::nullopt},
            {"KFC_MAX_CLIENTS", std::nullopt},
        };
        const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
        CHECK_EQ(config.server.port, 8765);
    }

    SUBCASE("OutOfRangePort") {
        const ScopedEnvironment env{
            {"KFC_PORT", "70000"},
            {"KFC_BIND_ADDRESS", std::nullopt},
            {"KFC_MAX_CLIENTS", std::nullopt},
        };
        const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
        CHECK_EQ(config.server.port, 8765);
    }
}

TEST_CASE("ConfigLoaderTest - IgnoresZeroMaxClients") {
    const ScopedEnvironment env{
        {"KFC_PORT", std::nullopt},
        {"KFC_BIND_ADDRESS", std::nullopt},
        {"KFC_MAX_CLIENTS", "0"},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK_EQ(config.server.max_clients, kfc::app::ServerConfig::kDefaultMaxClients);
}

TEST_CASE("ConfigLoaderTest - IgnoresEmptyBindAddress") {
    const ScopedEnvironment env{
        {"KFC_PORT", std::nullopt},
        {"KFC_BIND_ADDRESS", ""},
        {"KFC_MAX_CLIENTS", std::nullopt},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK_EQ(config.server.bind_address, "127.0.0.1");
}

TEST_CASE("ConfigLoaderTest - OverridesHealthPort") {
    const ScopedEnvironment env{
        {"KFC_HEALTH_PORT", "9090"},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK_EQ(config.server.health_port, 9090);
}

TEST_CASE("ConfigLoaderTest - IgnoresInvalidHealthPort") {
    SUBCASE("NonNumeric") {
        const ScopedEnvironment env{
            {"KFC_HEALTH_PORT", "abc"},
        };
        const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
        CHECK_EQ(config.server.health_port, 8080);
    }

    SUBCASE("Zero") {
        const ScopedEnvironment env{
            {"KFC_HEALTH_PORT", "0"},
        };
        const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
        CHECK_EQ(config.server.health_port, 8080);
    }

    SUBCASE("OutOfRange") {
        const ScopedEnvironment env{
            {"KFC_HEALTH_PORT", "70000"},
        };
        const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
        CHECK_EQ(config.server.health_port, 8080);
    }
}

TEST_CASE("ConfigLoaderTest - OverridesServerIdAndRegion") {
    const ScopedEnvironment env{
        {"KFC_SERVER_ID", "game-eu-1"},
        {"KFC_REGION", "eu-west"},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK_EQ(config.server.server_id, "game-eu-1");
    CHECK_EQ(config.server.region, "eu-west");
}

TEST_CASE("ConfigLoaderTest - OverridesGameEndpoint") {
    const ScopedEnvironment env{
        {"KFC_GAME_ENDPOINT", "ws://games.example:8765"},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    CHECK_EQ(config.server.endpoint, "ws://games.example:8765");
}

TEST_CASE("ConfigLoaderTest - MissingVariablesKeepDefaults") {
    const ScopedEnvironment env{
        {"KFC_PORT", std::nullopt},
        {"KFC_BIND_ADDRESS", std::nullopt},
        {"KFC_MAX_CLIENTS", std::nullopt},
    };

    const kfc::app::AppConfig config = kfc::app::load_config_from_environment();
    check_default_server_config(config.server);
}
