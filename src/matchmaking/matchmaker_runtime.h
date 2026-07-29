#pragma once

#include "app/app_config.h"
#include "app/server_metrics.h"
#include "matchmaking/http/matchmaking_http_server.h"
#include "matchmaking/remote_matchmaking_service.h"

#include <atomic>
#include <chrono>
#include <string>

namespace kfc::matchmaking {

class GatewayNotifierHttpClient;
class IMatchmakingService;

}  // namespace kfc::matchmaking

namespace kfc {

class IGameAllocator;
class IRuntimeStore;

namespace matchmaking {

class MatchmakerRuntime {
public:
    MatchmakerRuntime(const app::AppConfig& config, IRuntimeStore& runtime_store,
                      IGameAllocator& game_allocator, IGameRepository& game_repository,
                      matchmaking::GatewayNotifierHttpClient& gateway_notifier);
    ~MatchmakerRuntime();

    void run();
    void tick_once();
    void request_stop() noexcept { stop_requested_.store(true, std::memory_order_relaxed); }

    [[nodiscard]] app::ServerMetrics metrics() const;
    [[nodiscard]] bool is_ready() const;

private:
    void maybe_publish_heartbeat();

    RemoteMatchmakingService matchmaking_service_;
    MatchmakingHttpServer http_server_;
    std::string server_id_;
    std::string region_;
    bool redis_enabled_ = false;
    IRuntimeStore& runtime_store_;
    std::chrono::seconds heartbeat_interval_{1};
    std::chrono::steady_clock::time_point started_at_{};
    std::chrono::steady_clock::time_point last_tick_{};
    std::chrono::steady_clock::time_point last_heartbeat_at_{};
    std::chrono::steady_clock::time_point last_queue_process_{};
    std::int64_t last_tick_duration_ms_ = 0;
    std::atomic<bool> stop_requested_{false};
};

}  // namespace matchmaking
}  // namespace kfc
