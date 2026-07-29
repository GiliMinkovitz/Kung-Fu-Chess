#pragma once

#include "app/observability/metric_counters.h"
#include "app/server_metrics.h"

#include <string>

namespace kfc::app::observability {

enum class ServiceKind {
    Gateway,
    Matchmaker,
    GameServer,
};

[[nodiscard]] std::string format_prometheus_metrics(ServiceKind service_kind,
                                                    const ServerMetrics& metrics,
                                                    const MetricCounters& counters,
                                                    std::size_t authenticated_players = 0,
                                                    std::size_t active_players = 0,
                                                    std::size_t active_game_servers = 0,
                                                    bool allocation_api_active = false);

}  // namespace kfc::app::observability
