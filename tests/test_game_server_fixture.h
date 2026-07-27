#pragma once

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
        : infrastructure(db_path),
          server(port, std::move(board), infrastructure.dependencies()) {}
};

}  // namespace kfc::test
