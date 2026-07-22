#pragma once

#include "server/game_room.h"
#include "server/matchmaking.h"
#include "server/player_session.h"
#include "server/websocket_server.h"

#include "database/player_repository.h"
#include "database/sqlite_database.h"
#include "model/board_model.h"

#include <chrono>
#include <cstddef>
#include <list>
#include <string>

namespace kfc {

class GameServer {
public:
    GameServer(unsigned short port, BoardModel default_board, const std::string& db_path = "kfc.db");

    void run();

    [[nodiscard]] WebSocketServer& websocket_server() noexcept;
    [[nodiscard]] Matchmaking& matchmaking() noexcept;
    [[nodiscard]] GameRoom& room() noexcept;
    [[nodiscard]] SqliteDatabase& database() noexcept;
    [[nodiscard]] PlayerRepository& player_repository() noexcept;

private:
    void accept_new_clients();
    void prune_sessions();
    void process_active_room(std::int64_t elapsed, std::chrono::steady_clock::time_point& last_tick);
    void finish_active_room();

    WebSocketServer websocket_server_;
    Matchmaking matchmaking_;
    GameRoom room_;
    SqliteDatabase database_;
    PlayerRepository player_repository_;
    std::list<PlayerSession> sessions_;
    std::size_t next_session_id_ = 0;
};

}  // namespace kfc
