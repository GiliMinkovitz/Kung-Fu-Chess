#include "matchmaking/matchmaker_runtime.h"

#include "app/i_runtime_store.h"
#include "matchmaking/http/gateway_notifier_http_client.h"

#include <chrono>
#include <iostream>
#include <thread>

namespace kfc::matchmaking {

MatchmakerRuntime::MatchmakerRuntime(const app::AppConfig& config, IRuntimeStore& runtime_store,
                                     IGameAllocator& game_allocator,
                                     IGameRepository& game_repository,
                                     GatewayNotifierHttpClient& gateway_notifier)
    : matchmaking_service_(runtime_store, game_repository, game_allocator, gateway_notifier,
                           config.matchmaking, config.server.gateway_server_id),
      http_server_(config.server.bind_address, config.server.matchmaker_port, matchmaking_service_),
      server_id_(config.server.server_id),
      region_(config.server.region),
      redis_enabled_(config.redis.enabled),
      runtime_store_(runtime_store),
      heartbeat_interval_(config.redis.heartbeat_interval) {
    last_tick_ = std::chrono::steady_clock::now();
    last_queue_process_ = last_tick_;
}

MatchmakerRuntime::~MatchmakerRuntime() {
    http_server_.stop();
}

app::ServerMetrics MatchmakerRuntime::metrics() const {
    app::ServerMetrics result;
    result.active_rooms = 0;
    result.connected_sessions = 0;
    result.matchmaking_queue = matchmaking_service_.waiting_count(region_);
    result.server_id = server_id_;
    result.region = region_;
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

bool MatchmakerRuntime::is_ready() const {
    if (redis_enabled_ && !runtime_store_.is_available()) {
        return false;
    }
    return !runtime_store_.list_game_servers().empty();
}

std::size_t MatchmakerRuntime::active_game_server_count() const {
    return runtime_store_.list_game_servers().size();
}

void MatchmakerRuntime::maybe_publish_heartbeat() {
    const auto now = std::chrono::steady_clock::now();
    if (last_heartbeat_at_.time_since_epoch().count() != 0 &&
        now - last_heartbeat_at_ < heartbeat_interval_) {
        return;
    }

    runtime_store_.publish_server_heartbeat(server_id_, region_, metrics());
    last_heartbeat_at_ = now;
}

void MatchmakerRuntime::tick_once() {
    http_server_.poll();

    const auto now = std::chrono::steady_clock::now();
    if (now - last_queue_process_ >= std::chrono::seconds(1)) {
        matchmaking_service_.process_queue(region_);
        last_queue_process_ = now;
    }

    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last_tick_).count();
    last_tick_duration_ms_ = elapsed;
    last_tick_ = now;

    maybe_publish_heartbeat();

    if (elapsed < 16) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void MatchmakerRuntime::run() {
#ifndef KFC_TEST_BUILD
    std::cout << "Matchmaker started on port " << http_server_.port() << '\n';
#endif
    last_tick_ = std::chrono::steady_clock::now();
    started_at_ = last_tick_;
    maybe_publish_heartbeat();

    while (!stop_requested_.load(std::memory_order_relaxed)) {
        tick_once();
    }

    runtime_store_.deregister_server(server_id_);
}

}  // namespace kfc::matchmaking
