#pragma once

#include "app/app_config.h"
#include "app/server_infrastructure.h"
#include "model/board_model.h"
#include "server/game_server.h"

#include <utility>

namespace kfc::app {

struct BuiltGameServer {
    ServerInfrastructure infrastructure;
    GameServer server;

    BuiltGameServer(const AppConfig& config, BoardModel default_board)
        : infrastructure(config.database),
          server(config.server, std::move(default_board), infrastructure.dependencies()) {}
};

[[nodiscard]] inline BuiltGameServer build_game_server(const AppConfig& config, BoardModel default_board) {
    return BuiltGameServer{config, std::move(default_board)};
}

}  // namespace kfc::app
