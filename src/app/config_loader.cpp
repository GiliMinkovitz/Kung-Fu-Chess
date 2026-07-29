#include "app/config_loader.h"

#include "app/database_config.h"
#include "app/logging_config.h"
#include "app/matchmaking_config.h"
#include "app/redis_config.h"

#include <chrono>
#include <cstdlib>
#include <optional>
#include <string_view>

namespace {

std::optional<const char*> read_environment(const char* name) {
    const char* value = std::getenv(name);
    if (value == nullptr || value[0] == '\0') {
        return std::nullopt;
    }
    return value;
}

std::optional<unsigned long> parse_unsigned(std::string_view value) {
    if (value.empty()) {
        return std::nullopt;
    }

    unsigned long result = 0;
    for (const char c : value) {
        if (c < '0' || c > '9') {
            return std::nullopt;
        }
        const unsigned long next = result * 10UL + static_cast<unsigned long>(c - '0');
        if (next < result) {
            return std::nullopt;
        }
        result = next;
    }
    return result;
}

bool equals_ignore_case(std::string_view lhs, std::string_view rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (std::size_t i = 0; i < lhs.size(); ++i) {
        auto lower = [](char c) {
            if (c >= 'A' && c <= 'Z') {
                return static_cast<char>(c - 'A' + 'a');
            }
            return c;
        };
        if (lower(lhs[i]) != lower(rhs[i])) {
            return false;
        }
    }
    return true;
}

std::optional<kfc::app::DatabaseBackend> parse_database_backend(std::string_view value) {
    if (equals_ignore_case(value, "sqlite")) {
        return kfc::app::DatabaseBackend::SQLite;
    }
    if (equals_ignore_case(value, "postgres")) {
        return kfc::app::DatabaseBackend::PostgreSQL;
    }
    return std::nullopt;
}

std::optional<bool> parse_bool(std::string_view value) {
    if (equals_ignore_case(value, "true") || equals_ignore_case(value, "1") ||
        equals_ignore_case(value, "yes") || equals_ignore_case(value, "on")) {
        return true;
    }
    if (equals_ignore_case(value, "false") || equals_ignore_case(value, "0") ||
        equals_ignore_case(value, "no") || equals_ignore_case(value, "off")) {
        return false;
    }
    return std::nullopt;
}

void override_server_config(kfc::app::ServerConfig& server) {
    if (const auto port_value = read_environment("KFC_PORT")) {
        if (const auto port = parse_unsigned(*port_value)) {
            if (*port >= 1 && *port <= 65535) {
                server.port = static_cast<unsigned short>(*port);
            }
        }
    }

    if (const auto bind_value = read_environment("KFC_BIND_ADDRESS")) {
        server.bind_address = *bind_value;
    }

    if (const auto max_clients_value = read_environment("KFC_MAX_CLIENTS")) {
        if (const auto max_clients = parse_unsigned(*max_clients_value)) {
            if (*max_clients > 0) {
                server.max_clients = static_cast<std::size_t>(*max_clients);
            }
        }
    }

    if (const auto health_port_value = read_environment("KFC_HEALTH_PORT")) {
        if (const auto health_port = parse_unsigned(*health_port_value)) {
            if (*health_port >= 1 && *health_port <= 65535) {
                server.health_port = static_cast<unsigned short>(*health_port);
            }
        }
    }

    if (const auto server_id_value = read_environment("KFC_SERVER_ID")) {
        server.server_id = *server_id_value;
    }

    if (const auto region_value = read_environment("KFC_REGION")) {
        server.region = *region_value;
    }

    if (const auto endpoint_value = read_environment("KFC_GAME_ENDPOINT")) {
        server.endpoint = *endpoint_value;
    }
}

void override_database_config(kfc::app::DatabaseConfig& database) {
    if (const auto backend_value = read_environment("KFC_DB_BACKEND")) {
        if (const auto backend = parse_database_backend(*backend_value)) {
            database.backend = *backend;
        }
    }

    if (const auto path_value = read_environment("KFC_DB_PATH")) {
        database.path = *path_value;
    }

    if (const auto host_value = read_environment("KFC_DB_HOST")) {
        database.host = *host_value;
    }

    if (const auto port_value = read_environment("KFC_DB_PORT")) {
        if (const auto port = parse_unsigned(*port_value)) {
            if (*port >= 1 && *port <= 65535) {
                database.port = static_cast<int>(*port);
            }
        }
    }

    if (const auto database_value = read_environment("KFC_DB_NAME")) {
        database.database = *database_value;
    }

    if (const auto username_value = read_environment("KFC_DB_USER")) {
        database.username = *username_value;
    }

    if (const auto password_value = read_environment("KFC_DB_PASSWORD")) {
        database.password = *password_value;
    }
}

void override_matchmaking_config(kfc::app::MatchmakingConfig& matchmaking) {
    if (const auto rating_diff_value = read_environment("KFC_MATCH_MAX_RATING_DIFF")) {
        if (const auto rating_diff = parse_unsigned(*rating_diff_value)) {
            if (*rating_diff > 0) {
                matchmaking.max_rating_difference = static_cast<int>(*rating_diff);
            }
        }
    }

    if (const auto queue_timeout_value = read_environment("KFC_MATCH_QUEUE_TIMEOUT_SEC")) {
        if (const auto queue_timeout = parse_unsigned(*queue_timeout_value)) {
            if (*queue_timeout > 0) {
                matchmaking.queue_timeout = std::chrono::seconds(*queue_timeout);
            }
        }
    }
}

void override_logging_config(kfc::app::LoggingConfig& logging) {
    if (const auto diagnostics_value = read_environment("KFC_DIAGNOSTICS")) {
        if (const auto diagnostics_enabled = parse_bool(*diagnostics_value)) {
            logging.diagnostics_enabled = *diagnostics_enabled;
        }
    }
}

void override_redis_config(kfc::app::RedisConfig& redis) {
    if (const auto enabled_value = read_environment("KFC_REDIS_ENABLED")) {
        if (const auto enabled = parse_bool(*enabled_value)) {
            redis.enabled = *enabled;
        }
    }

    if (const auto host_value = read_environment("KFC_REDIS_HOST")) {
        redis.host = *host_value;
    }

    if (const auto port_value = read_environment("KFC_REDIS_PORT")) {
        if (const auto port = parse_unsigned(*port_value)) {
            if (*port >= 1 && *port <= 65535) {
                redis.port = static_cast<int>(*port);
            }
        }
    }

    if (const auto password_value = read_environment("KFC_REDIS_PASSWORD")) {
        redis.password = *password_value;
    }

    if (const auto database_value = read_environment("KFC_REDIS_DATABASE")) {
        if (const auto database = parse_unsigned(*database_value)) {
            redis.database = static_cast<int>(*database);
        }
    }
}

}  // namespace

namespace kfc::app {

AppConfig load_config_from_environment() {
    AppConfig config = make_default_config();
    override_server_config(config.server);
    override_database_config(config.database);
    override_matchmaking_config(config.matchmaking);
    override_logging_config(config.logging);
    override_redis_config(config.redis);
    return config;
}

}  // namespace kfc::app
