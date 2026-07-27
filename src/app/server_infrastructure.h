#pragma once

#include "app/database_config.h"
#include "app/game_server_dependencies.h"
#include "server/authentication_service.h"
#include "server/database/i_user_repository.h"

#include "database/i_database_connection.h"
#include "database/i_game_repository.h"
#include "database/sqlite_database.h"

#include <memory>

namespace kfc::app {

class ServerInfrastructure {
public:
    explicit ServerInfrastructure(const DatabaseConfig& database_config);

    [[nodiscard]] GameServerDependencies dependencies() noexcept;

    [[nodiscard]] IDatabaseConnection& database() noexcept;
    [[nodiscard]] IUserRepository& user_repository() noexcept;
    [[nodiscard]] IGameRepository& game_repository() noexcept;
    [[nodiscard]] AuthenticationService& authentication_service() noexcept;

private:
    SqliteDatabase database_;
    std::unique_ptr<IUserRepository> user_repository_;
    std::unique_ptr<IGameRepository> game_repository_;
    AuthenticationService authentication_service_;
};

}  // namespace kfc::app
