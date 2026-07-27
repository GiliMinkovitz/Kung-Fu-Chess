#include "app/server_infrastructure.h"

#include "database/sqlite_game_repository.h"
#include "server/database/sqlite_user_repository.h"

#include <stdexcept>

namespace kfc::app {

ServerInfrastructure::ServerInfrastructure(const DatabaseConfig& database_config)
    : database_(database_config.path),
      user_repository_(std::make_unique<SqliteUserRepository>(database_)),
      game_repository_(std::make_unique<SqliteGameRepository>(database_)),
      authentication_service_(*user_repository_) {
    if (!database_.open() || !database_.initialize_schema()) {
        throw std::runtime_error("Failed to initialize database");
    }
}

GameServerDependencies ServerInfrastructure::dependencies() noexcept {
    return GameServerDependencies{
        database_,
        *user_repository_,
        *game_repository_,
        authentication_service_,
    };
}

IDatabaseConnection& ServerInfrastructure::database() noexcept {
    return database_;
}

IUserRepository& ServerInfrastructure::user_repository() noexcept {
    return *user_repository_;
}

IGameRepository& ServerInfrastructure::game_repository() noexcept {
    return *game_repository_;
}

AuthenticationService& ServerInfrastructure::authentication_service() noexcept {
    return authentication_service_;
}

}  // namespace kfc::app
