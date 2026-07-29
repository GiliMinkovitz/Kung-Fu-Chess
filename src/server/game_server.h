#pragma once

#include "app/app_config.h"
#include "app/game_server_dependencies.h"
#include "app/server_metrics.h"
#include "server/game_result/game_result_handler.h"
#include "server/lobby/lobby_message_handler.h"
#include "server/match/match_lifecycle_handler.h"
#include "server/matchmaking/matchmaking_service.h"
#include "server/network/session_message_sink.h"
#include "server/network/game_input_dispatcher.h"
#include "server/room/active_room_processor.h"
#include "server/room/room_manager.h"
#include "server/database/i_user_repository.h"
#include "server/session/client_session_manager.h"
#include "server/session_registry.h"
#include "server/websocket_server.h"

#include "model/board_model.h"
#include "model/piece.h"

#include <atomic>
#include <chrono>
#include <optional>
#include <string>

namespace kfc {

class IRuntimeStore;

class GameServer {
public:
    GameServer(const app::AppConfig& config, BoardModel default_board,
               app::GameServerDependencies dependencies);
    ~GameServer();

    void run();
    void tick_once();
    void request_stop() noexcept { stop_requested_.store(true, std::memory_order_relaxed); }
#ifdef KFC_TEST_BUILD
    void finish_room(RoomId room_id, std::optional<PieceColor> winner_color, FinishReason reason) {
        game_result_handler_.finish(room_id, winner_color, reason);
    }
#endif

    [[nodiscard]] WebSocketServer& websocket_server() noexcept;
    [[nodiscard]] MatchmakingService& matchmaking_service() noexcept;
    [[nodiscard]] RoomManager& room_manager() noexcept;
    [[nodiscard]] IUserRepository& user_repository() noexcept;
    [[nodiscard]] IRuntimeStore& runtime_store() noexcept;
    [[nodiscard]] app::ServerMetrics metrics() const;

private:
    void maybe_publish_heartbeat();

    WebSocketServer websocket_server_;
    RoomManager room_manager_;
    std::string server_id_;
    std::string region_;
    MatchLifecycleHandler match_lifecycle_handler_;
    MatchmakingService matchmaking_service_;
    SessionRegistry session_registry_;
    ClientSessionManager session_manager_;
    SessionMessageSink session_message_sink_;
    GameInputDispatcher game_input_dispatcher_;
    ActiveRoomProcessor active_room_processor_;
    IUserRepository& user_repository_;
    IRuntimeStore& runtime_store_;
    LobbyMessageHandler lobby_handler_;
    GameResultHandler game_result_handler_;
    bool redis_enabled_ = false;
    std::chrono::seconds heartbeat_interval_{1};
    std::chrono::steady_clock::time_point started_at_{};
    std::chrono::steady_clock::time_point last_tick_{};
    std::chrono::steady_clock::time_point last_heartbeat_at_{};
    std::int64_t last_tick_duration_ms_ = 0;
    std::atomic<bool> stop_requested_{false};
};

}  // namespace kfc
