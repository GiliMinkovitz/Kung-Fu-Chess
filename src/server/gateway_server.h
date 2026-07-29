#pragma once

#include "app/app_config.h"
#include "app/server_metrics.h"
#include "server/client_connection_plane.h"
#include "server/gateway/local_game_gateway.h"
#include "server/lobby/lobby_message_handler.h"
#include "server/websocket_server.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <string>

namespace kfc {

class AuthenticationService;
class IRuntimeStore;
class MatchLifecycleHandler;

namespace matchmaking {
class IMatchmakingJoinClient;
}

class GatewayServer {
public:
    GatewayServer(const app::AppConfig& config, ClientConnectionPlane& client_plane,
                  LocalGameGateway& game_gateway,
                  matchmaking::IMatchmakingJoinClient& matchmaking_client,
                  AuthenticationService& authentication_service, IRuntimeStore& runtime_store,
                  std::function<void()> notification_poll = {},
                  std::function<void()> notification_stop = {});

    GatewayServer(const app::AppConfig& config, ClientConnectionPlane& client_plane,
                  LocalGameGateway& game_gateway,
                  matchmaking::IMatchmakingJoinClient& matchmaking_client,
                  MatchLifecycleHandler& match_lifecycle_handler,
                  AuthenticationService& authentication_service, IRuntimeStore& runtime_store);

    void run();
    void tick_once();
    void request_stop() noexcept { stop_requested_.store(true, std::memory_order_relaxed); }

    [[nodiscard]] WebSocketServer& websocket_server() noexcept;
    [[nodiscard]] app::ServerMetrics metrics() const;

private:
    void maybe_publish_heartbeat();

    ClientConnectionPlane& client_plane_;
    IRuntimeStore& runtime_store_;
    LocalGameGateway& local_game_gateway_;
    LobbyMessageHandler lobby_handler_;
    std::function<void()> notification_poll_;
    std::function<void()> notification_stop_;
    MatchLifecycleHandler* match_lifecycle_handler_ = nullptr;
    std::string gateway_server_id_;
    std::string region_;
    std::string game_endpoint_;
    bool redis_enabled_ = false;
    std::chrono::seconds heartbeat_interval_{1};
    std::chrono::steady_clock::time_point started_at_{};
    std::chrono::steady_clock::time_point last_tick_{};
    std::chrono::steady_clock::time_point last_heartbeat_at_{};
    std::int64_t last_tick_duration_ms_ = 0;
    std::atomic<bool> stop_requested_{false};
};

}  // namespace kfc
