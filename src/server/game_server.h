#pragma once

#include "model/board_model.h"
#include "server/game_room.h"
#include "server/matchmaking.h"
#include "server/websocket_server.h"

#include <memory>
#include <vector>

namespace kfc {

class GameServer {
public:
    explicit GameServer(unsigned short port, BoardModel default_board);

    [[nodiscard]] WebSocketServer& websocket_server() noexcept;
    [[nodiscard]] Matchmaking& matchmaking() noexcept;
    [[nodiscard]] const BoardModel& default_board() const noexcept;
    [[nodiscard]] const std::vector<std::unique_ptr<GameRoom>>& rooms() const noexcept;

private:
    BoardModel default_board_;
    WebSocketServer websocket_server_;
    Matchmaking matchmaking_;
    std::vector<std::unique_ptr<GameRoom>> rooms_;
};

}  // namespace kfc
