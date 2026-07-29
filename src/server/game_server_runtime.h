#pragma once

#include "app/app_config.h"
#include "app/server_metrics.h"
#include "server/client_connection_plane.h"
#include "server/game/game_allocation_handler.h"
#include "server/game/game_creation_listener.h"
#include "server/game/game_join_handler.h"
#include "server/game/local_game_host.h"
#include "server/game_result/game_result_handler.h"
#include "server/gateway/local_game_completion_gateway.h"
#include "server/network/game_input_dispatcher.h"
#include "server/room/active_room_processor.h"
#include "server/room/room_manager.h"

#include "model/piece.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <string>

namespace kfc {

class IGameRepository;
class IRuntimeStore;
class IUserRepository;

class GameServerRuntime {
public:
    GameServerRuntime(const app::AppConfig& config, ClientConnectionPlane& client_plane,
                      RoomManager& room_manager, LocalGameHost& local_game_host,
                      GameAllocationHandler& allocation_handler, IUserRepository& user_repository,
                      IGameRepository& game_repository, IRuntimeStore& runtime_store,
                      bool enable_creation_listener = true, bool manage_connections = true);
    ~GameServerRuntime();

    void run();
    void tick_once();
    void request_stop() noexcept { stop_requested_.store(true, std::memory_order_relaxed); }

    [[nodiscard]] RoomManager& room_manager() noexcept;
    [[nodiscard]] app::ServerMetrics metrics() const;

#ifdef KFC_TEST_BUILD
    void finish_room(RoomId room_id, std::optional<PieceColor> winner_color, FinishReason reason);
#endif

private:
    void maybe_publish_heartbeat();
    void finish_room_internal(RoomId room_id, std::optional<PieceColor> winner_color,
                              FinishReason reason);

    ClientConnectionPlane& client_plane_;
    RoomManager& room_manager_;
    LocalGameHost& local_game_host_;
    GameAllocationHandler& allocation_handler_;
    IRuntimeStore& runtime_store_;
    GameInputDispatcher input_dispatcher_;
    LocalGameCompletionGateway completion_gateway_;
    GameResultHandler game_result_handler_;
    ActiveRoomProcessor active_room_processor_;
    GameJoinHandler game_join_handler_;
    std::unique_ptr<GameCreationListener> creation_listener_;
    std::string server_id_;
    std::string region_;
    std::string endpoint_;
    std::string allocation_endpoint_;
    bool redis_enabled_ = false;
    bool manage_connections_ = true;
    std::chrono::seconds heartbeat_interval_{1};
    std::chrono::steady_clock::time_point started_at_{};
    std::chrono::steady_clock::time_point last_tick_{};
    std::chrono::steady_clock::time_point last_heartbeat_at_{};
    std::int64_t last_tick_duration_ms_ = 0;
    std::atomic<bool> stop_requested_{false};
};

}  // namespace kfc
