#include "app/observability/prometheus_formatter.h"

#include <sstream>

namespace kfc::app::observability {

namespace {

void append_counter(std::ostringstream& stream, std::string_view name, std::string_view help,
                    const std::uint64_t value) {
    stream << "# HELP " << name << ' ' << help << '\n';
    stream << "# TYPE " << name << " counter\n";
    stream << name << ' ' << value << '\n';
}

void append_gauge(std::ostringstream& stream, std::string_view name, std::string_view help,
                  const std::uint64_t value) {
    stream << "# HELP " << name << ' ' << help << '\n';
    stream << "# TYPE " << name << " gauge\n";
    stream << name << ' ' << value << '\n';
}

void append_histogram_summary(std::ostringstream& stream, std::string_view name,
                              std::string_view help, const std::uint64_t sum_ms,
                              const std::uint64_t count) {
    stream << "# HELP " << name << ' ' << help << '\n';
    stream << "# TYPE " << name << " summary\n";
    stream << name << "_sum " << (static_cast<double>(sum_ms) / 1000.0) << '\n';
    stream << name << "_count " << count << '\n';
}

}  // namespace

std::string format_prometheus_metrics(const ServiceKind service_kind, const ServerMetrics& metrics,
                                      const MetricCounters& counters,
                                      const std::size_t authenticated_players,
                                      const std::size_t active_players,
                                      const std::size_t active_game_servers,
                                      const bool allocation_api_active) {
    std::ostringstream stream;

    append_gauge(stream, "server_uptime_seconds", "Process uptime in seconds",
                 static_cast<std::uint64_t>(metrics.server_uptime_seconds));
    append_gauge(stream, "redis_enabled", "Whether Redis is enabled",
                 metrics.redis_enabled ? 1U : 0U);
    append_gauge(stream, "redis_connected", "Whether Redis is connected",
                 metrics.redis_connected ? 1U : 0U);

    switch (service_kind) {
        case ServiceKind::Gateway:
            append_gauge(stream, "connected_sessions", "Active gateway lobby sessions",
                         metrics.connected_sessions);
            append_gauge(stream, "websocket_connections", "Active gateway websocket connections",
                         metrics.connected_sessions);
            append_gauge(stream, "authenticated_players", "Authenticated players in gateway lobby",
                         authenticated_players);
            append_counter(stream, "matchmaking_requests_total",
                           "Total matchmaking join requests forwarded by gateway",
                           counters.matchmaking_requests_total.load(std::memory_order_relaxed));
            append_counter(stream, "matchmaking_failures_total",
                           "Total failed matchmaking join requests from gateway",
                           counters.matchmaking_failures_total.load(std::memory_order_relaxed));
            break;
        case ServiceKind::Matchmaker:
            append_gauge(stream, "queue_size", "Players waiting in matchmaking queue",
                         metrics.matchmaking_queue);
            append_gauge(stream, "active_game_servers", "Registered healthy game servers",
                         active_game_servers);
            append_counter(stream, "matches_created_total", "Total matches created",
                           counters.matches_created_total.load(std::memory_order_relaxed));
            append_counter(stream, "allocation_failures_total",
                           "Total game allocation failures in matchmaker",
                           counters.allocation_failures_total.load(std::memory_order_relaxed));
            append_histogram_summary(
                stream, "matchmaking_duration_seconds", "Matchmaking finalize duration",
                counters.matchmaking_duration_ms_total.load(std::memory_order_relaxed),
                counters.matchmaking_duration_count.load(std::memory_order_relaxed));
            break;
        case ServiceKind::GameServer:
            append_gauge(stream, "active_rooms", "Active game rooms", metrics.active_rooms);
            append_gauge(stream, "active_players", "Connected players in active rooms",
                         active_players);
            append_gauge(stream, "connected_sessions", "Active game websocket sessions",
                         metrics.connected_sessions);
            append_gauge(stream, "allocation_api_active", "Whether allocation API listener is active",
                         allocation_api_active ? 1U : 0U);
            append_counter(stream, "snapshots_sent_total", "Total game snapshots sent to clients",
                           counters.snapshots_sent_total.load(std::memory_order_relaxed));
            append_counter(stream, "player_actions_total", "Total accepted player actions",
                           counters.player_actions_total.load(std::memory_order_relaxed));
            append_counter(stream, "allocation_requests_total",
                           "Total allocation requests handled by game server",
                           counters.allocation_requests_total.load(std::memory_order_relaxed));
            append_histogram_summary(
                stream, "tick_duration_seconds", "Game server tick duration",
                counters.tick_duration_ms_total.load(std::memory_order_relaxed),
                counters.tick_duration_count.load(std::memory_order_relaxed));
            append_gauge(stream, "last_tick_duration_ms", "Most recent tick duration in milliseconds",
                         static_cast<std::uint64_t>(metrics.last_tick_duration_ms));
            break;
    }

    return stream.str();
}

}  // namespace kfc::app::observability
