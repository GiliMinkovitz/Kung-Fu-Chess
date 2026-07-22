#pragma once

#include "server/game_room.h"
#include "server/matchmaking.h"
#include "server/websocket_server.h"

#include "model/board_model.h"

namespace kfc {

class GameServer {
public:
    explicit GameServer(unsigned short port, BoardModel default_board);

    void run();

    [[nodiscard]] WebSocketServer& websocket_server() noexcept;
    [[nodiscard]] Matchmaking& matchmaking() noexcept;
    [[nodiscard]] GameRoom& room() noexcept;

private:
    WebSocketServer websocket_server_;
    Matchmaking matchmaking_;
    GameRoom room_;
};

}  // namespace kfc
