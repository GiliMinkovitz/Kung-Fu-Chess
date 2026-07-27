#pragma once

#include "app/server_infrastructure.h"
#include "model/board_model.h"
#include "server/game_server.h"

#include <memory>
#include <utility>

namespace kfc::app {

struct BuiltGameServer {
    ServerInfrastructure infrastructure;
    GameServer server;

    BuiltGameServer(unsigned short port, BoardModel default_board, const std::string& db_path)
        : infrastructure(db_path),
          server(port, std::move(default_board), infrastructure.dependencies()) {}
};

[[nodiscard]] inline BuiltGameServer build_game_server(unsigned short port, BoardModel default_board,
                                                       const std::string& db_path) {
    return BuiltGameServer{port, std::move(default_board), db_path};
}

}  // namespace kfc::app
