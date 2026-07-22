#pragma once

#include "server/game_room.h"
#include "server/matchmaking.h"
#include "server/player_session.h"
#include "server/websocket_server.h"

#include "model/board_model.h"

#include <chrono>
#include <cstddef>
#include <list>

namespace kfc {

class GameServer {
public:
    explicit GameServer(unsigned short port, BoardModel default_board);

    void run();

    [[nodiscard]] WebSocketServer& websocket_server() noexcept;
    [[nodiscard]] Matchmaking& matchmaking() noexcept;
    [[nodiscard]] GameRoom& room() noexcept;

private:
    void accept_new_clients();
    void prune_sessions();
    void process_active_room(std::int64_t elapsed, std::chrono::steady_clock::time_point& last_tick);

    WebSocketServer websocket_server_;
    Matchmaking matchmaking_;
    GameRoom room_;
    std::list<PlayerSession> sessions_;
    std::size_t next_session_id_ = 0;
};

}  // namespace kfc
