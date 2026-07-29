#pragma once

#include <atomic>
#include <cstdint>

namespace kfc::app::observability {

struct MetricCounters {
    std::atomic<std::uint64_t> matchmaking_requests_total{0};
    std::atomic<std::uint64_t> matchmaking_failures_total{0};
    std::atomic<std::uint64_t> matches_created_total{0};
    std::atomic<std::uint64_t> allocation_failures_total{0};
    std::atomic<std::uint64_t> matchmaking_duration_ms_total{0};
    std::atomic<std::uint64_t> matchmaking_duration_count{0};
    std::atomic<std::uint64_t> snapshots_sent_total{0};
    std::atomic<std::uint64_t> player_actions_total{0};
    std::atomic<std::uint64_t> allocation_requests_total{0};
    std::atomic<std::uint64_t> tick_duration_ms_total{0};
    std::atomic<std::uint64_t> tick_duration_count{0};
};

[[nodiscard]] MetricCounters& metrics();

void record_matchmaking_duration_ms(std::uint64_t duration_ms);
void record_tick_duration_ms(std::uint64_t duration_ms);

}  // namespace kfc::app::observability
