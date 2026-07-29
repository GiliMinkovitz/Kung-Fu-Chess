#pragma once

#include "app/app_config.h"
#include "app/game_server_dependencies.h"
#include "app/server_metrics.h"
#include "matchmaking/local_matchmaking_join_client.h"
#include "server/client_connection_plane.h"
#include "server/game/local_game_allocator.h"
#include "server/game/local_game_host.h"
#include "server/game/game_allocation_handler.h"
#include "server/gateway/local_game_gateway.h"
#include "server/gateway/monolith_matchmaking_disconnect_handler.h"
#include "server/gateway_server.h"
#include "server/game_server_runtime.h"
#include "server/match/match_lifecycle_handler.h"
#include "server/matchmaking/matchmaking_service.h"
#include "server/websocket_server.h"
#include "server/database/i_user_repository.h"
#include "server/room/room_manager.h"

#include "model/board_model.h"
#include "model/piece.h"

#include <atomic>
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
    void request_stop() noexcept;
#ifdef KFC_TEST_BUILD
    void finish_room(RoomId room_id, std::optional<PieceColor> winner_color, FinishReason reason);
#endif

    [[nodiscard]] WebSocketServer& websocket_server() noexcept;
    [[nodiscard]] MatchmakingService& matchmaking_service() noexcept;
    [[nodiscard]] RoomManager& room_manager() noexcept;
    [[nodiscard]] IUserRepository& user_repository() noexcept;
    [[nodiscard]] IRuntimeStore& runtime_store() noexcept;
    [[nodiscard]] app::ServerMetrics metrics() const;

private:
    RoomManager room_manager_;
    LocalGameHost local_game_host_;
    GameAllocationHandler allocation_handler_;
    LocalGameAllocator local_game_allocator_;
    MatchLifecycleHandler match_lifecycle_handler_;
    MatchmakingService matchmaking_service_;
    MonolithMatchmakingDisconnectHandler disconnect_handler_;
    ClientConnectionPlane client_plane_;
    matchmaking::LocalMatchmakingJoinClient matchmaking_join_client_;
    LocalGameGateway local_game_gateway_;
    GatewayServer gateway_;
    GameServerRuntime runtime_;
    IUserRepository& user_repository_;
    IRuntimeStore& runtime_store_;
    std::atomic<bool> stop_requested_{false};
};

}  // namespace kfc
