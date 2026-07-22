#pragma once

#include "server/player_session.h"

#include <chrono>
#include <cstddef>
#include <optional>
#include <vector>

namespace kfc {

class Matchmaking {
public:
    static constexpr int kMaxRatingDifference = 100;
    static constexpr std::chrono::seconds kQueueTimeout{60};

    struct WaitingEntry {
        PlayerSession* session;
        std::chrono::steady_clock::time_point enqueue_time;
    };

    void remove(PlayerSession& session);

    [[nodiscard]] std::optional<std::vector<PlayerSession*>> enqueue(
        PlayerSession& session,
        std::chrono::steady_clock::time_point now);

    [[nodiscard]] std::vector<PlayerSession*> check_timeouts(
        std::chrono::steady_clock::time_point now);

    [[nodiscard]] std::size_t waiting_count() const noexcept;

private:
    [[nodiscard]] static bool are_compatible(const PlayerSession& a, const PlayerSession& b);
    [[nodiscard]] static bool is_eligible(const PlayerSession& session);

    std::vector<WaitingEntry> waiting_;
};

}  // namespace kfc
