#pragma once

#include "server/matchmaking/match_created_handler.h"
#include "server/matchmaking/matchmaking_queue.h"

#include <chrono>
#include <cstddef>
#include <optional>
#include <vector>

namespace kfc {

class MatchmakingService {
public:
    explicit MatchmakingService(IMatchCreatedHandler& handler);

    [[nodiscard]] std::optional<MatchCreated> enqueue(
        PlayerSession& session,
        std::chrono::steady_clock::time_point now);

    void remove(PlayerSession& session);

    [[nodiscard]] std::vector<PlayerSession*> check_timeouts(
        std::chrono::steady_clock::time_point now);

    [[nodiscard]] std::size_t waiting_count() const noexcept;

#ifdef KFC_TEST_BUILD
    void set_queue_timeout(std::chrono::milliseconds timeout);
#endif

private:
    [[nodiscard]] MatchCreated finalize_match(PlayerSession* white, PlayerSession* black);

    MatchmakingQueue queue_;
    IMatchCreatedHandler& handler_;
};

}  // namespace kfc
