#include "server/game_server.h"

namespace kfc {

GameServer::GameServer(unsigned short port, BoardModel default_board)
    : default_board_(std::move(default_board)), websocket_server_(port) {}

WebSocketServer& GameServer::websocket_server() noexcept {
    return websocket_server_;
}

Matchmaking& GameServer::matchmaking() noexcept {
    return matchmaking_;
}

const BoardModel& GameServer::default_board() const noexcept {
    return default_board_;
}

const std::vector<std::unique_ptr<GameRoom>>& GameServer::rooms() const noexcept {
    return rooms_;
}

}  // namespace kfc
