#pragma once

#include "app/game_server_dependencies.h"
#include "app/server_config.h"
#include "server/game_result/game_result_handler.h"
#include "server/lobby/lobby_message_handler.h"
#include "server/match/match_lifecycle_handler.h"
#include "server/matchmaking/matchmaking_service.h"
#include "server/room/active_room_processor.h"
#include "server/room/room_manager.h"
#include "server/database/i_user_repository.h"
#include "server/session/client_session_manager.h"
#include "server/session_registry.h"
#include "server/websocket_server.h"

#include "model/board_model.h"
#include "model/piece.h"

#include <chrono>
#include <optional>

namespace kfc {

class GameServer {
public:
    GameServer(const app::ServerConfig& server_config, BoardModel default_board,
               app::GameServerDependencies dependencies);
    ~GameServer();

    void run();
    void tick_once();
#ifdef KFC_TEST_BUILD
    void request_stop() noexcept { stop_requested_ = true; }
    void finish_room(RoomId room_id, std::optional<PieceColor> winner_color, FinishReason reason) {
        game_result_handler_.finish(room_id, winner_color, reason);
    }
#endif

    [[nodiscard]] WebSocketServer& websocket_server() noexcept;
    [[nodiscard]] MatchmakingService& matchmaking_service() noexcept;
    [[nodiscard]] RoomManager& room_manager() noexcept;
    [[nodiscard]] IUserRepository& user_repository() noexcept;

private:
    WebSocketServer websocket_server_;
    RoomManager room_manager_;
    MatchLifecycleHandler match_lifecycle_handler_;
    MatchmakingService matchmaking_service_;
    ActiveRoomProcessor active_room_processor_;
    IUserRepository& user_repository_;
    SessionRegistry session_registry_;
    ClientSessionManager session_manager_;
    LobbyMessageHandler lobby_handler_;
    GameResultHandler game_result_handler_;
    std::chrono::steady_clock::time_point last_tick_{};
#ifdef KFC_TEST_BUILD
    bool stop_requested_ = false;
#endif
};

}  // namespace kfc
