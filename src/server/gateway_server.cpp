#include "server/gateway_server.h"

#include "app/i_runtime_store.h"
#include "app/runtime_endpoint.h"
#include "model/game_config.h"
#include "server/authentication_service.h"
#include "server/gateway/local_game_gateway.h"
#include "server/player_session.h"

#include <chrono>
#include <iostream>
#include <thread>

namespace kfc {

GatewayServer::GatewayServer(const app::AppConfig& config, ClientConnectionPlane& client_plane,
                             LocalGameGateway& game_gateway,
                             matchmaking::IMatchmakingJoinClient& matchmaking_client,
                             AuthenticationService& authentication_service,
                             IRuntimeStore& runtime_store,
                             std::function<void()> notification_poll,
                             std::function<void()> notification_stop)
    : client_plane_(client_plane),
      runtime_store_(runtime_store),
      local_game_gateway_(game_gateway),
      lobby_handler_(authentication_service, matchmaking_client, client_plane_.session_registry,
                     client_plane_.session_manager, config.server.region),
      notification_poll_(std::move(notification_poll)),
      notification_stop_(std::move(notification_stop)),
      gateway_server_id_(config.server.gateway_server_id),
      region_(config.server.region),
      game_endpoint_(app::resolve_game_endpoint(config.server)),
      redis_enabled_(config.redis.enabled),
      heartbeat_interval_(config.redis.heartbeat_interval) {
    last_tick_ = std::chrono::steady_clock::now();
}

WebSocketServer& GatewayServer::websocket_server() noexcept {
    return client_plane_.websocket_server;
}

app::ServerMetrics GatewayServer::metrics() const {
    app::ServerMetrics result;
    result.active_rooms = 0;
    result.connected_sessions = client_plane_.session_manager.sessions().size();
    result.matchmaking_queue = 0;
    result.server_id = gateway_server_id_;
    result.region = region_;
    result.endpoint = game_endpoint_;
    result.redis_enabled = redis_enabled_;
    result.redis_connected = redis_enabled_ && runtime_store_.is_available();

    const auto now = std::chrono::steady_clock::now();
    if (started_at_.time_since_epoch().count() != 0) {
        result.server_uptime_seconds =
            std::chrono::duration_cast<std::chrono::seconds>(now - started_at_).count();
    }
    result.last_tick_duration_ms = last_tick_duration_ms_;
    return result;
}

std::size_t GatewayServer::authenticated_player_count() const {
    std::size_t count = 0;
    for (const PlayerSession& session : client_plane_.session_manager.sessions()) {
        if (session.has_user()) {
            ++count;
        }
    }
    return count;
}

void GatewayServer::maybe_publish_heartbeat() {
    const auto now = std::chrono::steady_clock::now();
    if (last_heartbeat_at_.time_since_epoch().count() != 0 &&
        now - last_heartbeat_at_ < heartbeat_interval_) {
        return;
    }

    runtime_store_.publish_server_heartbeat(gateway_server_id_, region_, metrics());
    last_heartbeat_at_ = now;
}

void GatewayServer::tick_once() {
    client_plane_.session_manager.accept_new_clients();
    lobby_handler_.process();

    if (notification_poll_) {
        notification_poll_();
    }

    if (process_match_timeouts_) {
        process_match_timeouts_();
    }

    const auto now = std::chrono::steady_clock::now();
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last_tick_).count();
    last_tick_duration_ms_ = elapsed;
    last_tick_ = now;

    client_plane_.session_manager.prune_sessions();
    maybe_publish_heartbeat();

    if (elapsed < kTargetFrameMs) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void GatewayServer::run() {
#ifndef KFC_TEST_BUILD
    std::cout << "Gateway started\n";
#endif
    last_tick_ = std::chrono::steady_clock::now();
    started_at_ = last_tick_;
    maybe_publish_heartbeat();

    while (!stop_requested_.load(std::memory_order_relaxed)) {
        tick_once();
    }

    if (notification_stop_) {
        notification_stop_();
    }

    runtime_store_.deregister_server(gateway_server_id_);
}

}  // namespace kfc
