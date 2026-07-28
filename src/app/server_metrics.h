#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace kfc::app {

struct ServerMetrics {
    std::size_t active_rooms = 0;
    std::size_t connected_sessions = 0;
    std::size_t matchmaking_queue = 0;
    std::int64_t server_uptime_seconds = 0;
    std::int64_t last_tick_duration_ms = 0;
};

[[nodiscard]] inline std::string format_server_metrics(const ServerMetrics& metrics) {
    return "active_rooms " + std::to_string(metrics.active_rooms) + "\n" +
           "connected_sessions " + std::to_string(metrics.connected_sessions) + "\n" +
           "matchmaking_queue " + std::to_string(metrics.matchmaking_queue) + "\n" +
           "server_uptime_seconds " + std::to_string(metrics.server_uptime_seconds) + "\n" +
           "last_tick_duration_ms " + std::to_string(metrics.last_tick_duration_ms) + "\n";
}

}  // namespace kfc::app
