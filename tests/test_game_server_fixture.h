#pragma once

#include "app/database_config.h"
#include "app/server_config.h"
#include "app/server_infrastructure.h"
#include "model/board_model.h"
#include "server/game_server.h"

#include <string>
#include <utility>

namespace kfc::test {

struct GameServerFixture {
    app::ServerInfrastructure infrastructure;
    GameServer server;

    GameServerFixture(unsigned short port, BoardModel board, const std::string& db_path = ":memory:")
        : infrastructure(make_database_config(db_path)),
          server(make_server_config(port), std::move(board), infrastructure.dependencies()) {}

private:
    static app::DatabaseConfig make_database_config(const std::string& db_path) {
        app::DatabaseConfig config;
        config.path = db_path;
        return config;
    }

    static app::ServerConfig make_server_config(unsigned short port) {
        app::ServerConfig config;
        config.port = port;
        return config;
    }
};

}  // namespace kfc::test
