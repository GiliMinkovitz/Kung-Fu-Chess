#include "app/observability/metric_counters.h"

namespace kfc::app::observability {

namespace {

MetricCounters g_metrics;

}  // namespace

MetricCounters& metrics() {
    return g_metrics;
}

void record_matchmaking_duration_ms(const std::uint64_t duration_ms) {
    g_metrics.matchmaking_duration_ms_total.fetch_add(duration_ms, std::memory_order_relaxed);
    g_metrics.matchmaking_duration_count.fetch_add(1, std::memory_order_relaxed);
}

void record_tick_duration_ms(const std::uint64_t duration_ms) {
    g_metrics.tick_duration_ms_total.fetch_add(duration_ms, std::memory_order_relaxed);
    g_metrics.tick_duration_count.fetch_add(1, std::memory_order_relaxed);
}

}  // namespace kfc::app::observability
