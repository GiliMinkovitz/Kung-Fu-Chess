#include "app/server_infrastructure.h"

#include "app/in_memory_runtime_store.h"
#include "app/redis_runtime_store.h"
#include "app/server_health.h"
#include "database/postgres_connection.h"
#include "database/postgres_game_repository.h"
#include "database/sqlite_database.h"
#include "database/sqlite_game_repository.h"
#include "server/database/postgres_user_repository.h"
#include "server/database/sqlite_user_repository.h"

#include <stdexcept>

namespace kfc::app {

namespace {

PostgresConnection::Settings make_postgres_settings(const DatabaseConfig& database_config) {
    return PostgresConnection::Settings{
        database_config.host,
        database_config.port,
        database_config.database,
        database_config.username,
        database_config.password,
    };
}

std::unique_ptr<IRuntimeStore> make_runtime_store(const RedisConfig& redis_config) {
    if (redis_config.enabled) {
        return std::make_unique<RedisRuntimeStore>(redis_config);
    }
    return std::make_unique<InMemoryRuntimeStore>();
}

}  // namespace

ServerInfrastructure::ServerInfrastructure(const AppConfig& config)
    : database_backend_(config.database.backend),
      redis_config_(config.redis),
      runtime_store_(make_runtime_store(config.redis)) {
    if (config.redis.enabled && !runtime_store_->is_available()) {
        throw std::runtime_error("Failed to connect to Redis");
    }

    if (config.database.backend == DatabaseBackend::PostgreSQL) {
        auto postgres = std::make_unique<PostgresConnection>(make_postgres_settings(config.database));
        if (!postgres->open()) {
            throw std::runtime_error("Failed to connect to PostgreSQL");
        }
        if (!postgres->initialize_schema()) {
            throw std::runtime_error("Failed to initialize PostgreSQL connection");
        }

        database_connection_ = std::move(postgres);
        auto& postgres_connection = static_cast<PostgresConnection&>(*database_connection_);
        user_repository_ = std::make_unique<PostgresUserRepository>(postgres_connection);
        game_repository_ = std::make_unique<PostgresGameRepository>(postgres_connection);
        authentication_service_ = std::make_unique<AuthenticationService>(*user_repository_);
        return;
    }

    auto sqlite = std::make_unique<SqliteDatabase>(config.database.path);
    if (!sqlite->open() || !sqlite->initialize_schema()) {
        throw std::runtime_error("Failed to initialize database");
    }

    database_connection_ = std::move(sqlite);
    user_repository_ = std::make_unique<SqliteUserRepository>(*database_connection_);
    game_repository_ = std::make_unique<SqliteGameRepository>(*database_connection_);
    authentication_service_ = std::make_unique<AuthenticationService>(*user_repository_);
}

GameServerDependencies ServerInfrastructure::dependencies() noexcept {
    return GameServerDependencies{
        *database_connection_,
        *user_repository_,
        *game_repository_,
        *authentication_service_,
        *runtime_store_,
    };
}

HealthStatus ServerInfrastructure::get_health_status(const bool server_running) const {
    return make_health_status(server_running, database_backend_, *database_connection_, redis_config_,
                              *runtime_store_);
}

IDatabaseConnection& ServerInfrastructure::database() noexcept {
    return *database_connection_;
}

IUserRepository& ServerInfrastructure::user_repository() noexcept {
    return *user_repository_;
}

IGameRepository& ServerInfrastructure::game_repository() noexcept {
    return *game_repository_;
}

AuthenticationService& ServerInfrastructure::authentication_service() noexcept {
    return *authentication_service_;
}

IRuntimeStore& ServerInfrastructure::runtime_store() noexcept {
    return *runtime_store_;
}

}  // namespace kfc::app
