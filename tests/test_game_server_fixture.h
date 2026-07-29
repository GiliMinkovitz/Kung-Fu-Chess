#pragma once

#include "app/app_config.h"
#include "app/database_config.h"
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
        : infrastructure(make_infrastructure_config(db_path)),
          server(make_app_config(port), std::move(board), infrastructure.dependencies()) {}

private:
    static app::AppConfig make_infrastructure_config(const std::string& db_path) {
        app::AppConfig config;
        config.database.path = db_path;
        return config;
    }

    static app::AppConfig make_app_config(unsigned short port) {
        app::AppConfig config;
        config.server.port = port;
        return config;
    }
};

}  // namespace kfc::test
