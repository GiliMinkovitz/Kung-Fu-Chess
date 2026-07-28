#pragma once

#include "server/player_session.h"

#include <chrono>
#include <cstddef>
#include <optional>
#include <vector>

namespace kfc {

class MatchmakingQueue {
public:
    MatchmakingQueue(int max_rating_difference, std::chrono::milliseconds queue_timeout);

    void set_queue_timeout(std::chrono::milliseconds timeout) { queue_timeout_ = timeout; }

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
    [[nodiscard]] bool are_compatible(const PlayerSession& a, const PlayerSession& b) const;
    [[nodiscard]] static bool is_eligible(const PlayerSession& session);

    int max_rating_difference_;
    std::chrono::milliseconds queue_timeout_;
    std::vector<WaitingEntry> waiting_;
};

}  // namespace kfc
