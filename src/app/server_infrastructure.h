#pragma once

#include "app/database_config.h"
#include "app/game_server_dependencies.h"
#include "app/health_status.h"
#include "server/authentication_service.h"
#include "server/database/i_user_repository.h"

#include "database/i_database_connection.h"
#include "database/i_game_repository.h"

#include <memory>

namespace kfc::app {

class ServerInfrastructure {
public:
    explicit ServerInfrastructure(const DatabaseConfig& database_config);

    [[nodiscard]] GameServerDependencies dependencies() noexcept;

    [[nodiscard]] HealthStatus get_health_status(bool server_running) const;

    [[nodiscard]] IDatabaseConnection& database() noexcept;
    [[nodiscard]] IUserRepository& user_repository() noexcept;
    [[nodiscard]] IGameRepository& game_repository() noexcept;
    [[nodiscard]] AuthenticationService& authentication_service() noexcept;

private:
    DatabaseBackend database_backend_;
    std::unique_ptr<IDatabaseConnection> database_connection_;
    std::unique_ptr<IUserRepository> user_repository_;
    std::unique_ptr<IGameRepository> game_repository_;
    std::unique_ptr<AuthenticationService> authentication_service_;
};

}  // namespace kfc::app
