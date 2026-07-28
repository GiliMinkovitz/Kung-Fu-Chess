#pragma once

#include "app/matchmaking_config.h"
#include "server/matchmaking/match_created_handler.h"
#include "server/matchmaking/matchmaking_queue.h"

#include <chrono>
#include <cstddef>
#include <optional>
#include <vector>

namespace kfc {

class MatchmakingService {
public:
    MatchmakingService(IMatchCreatedHandler& handler, const app::MatchmakingConfig& config);

    [[nodiscard]] std::optional<MatchCreated> enqueue(
        PlayerSession& session,
        std::chrono::steady_clock::time_point now);

    void remove(PlayerSession& session);

    [[nodiscard]] std::vector<PlayerSession*> check_timeouts(
        std::chrono::steady_clock::time_point now);

    [[nodiscard]] std::size_t waiting_count() const noexcept;

    void set_queue_timeout(std::chrono::milliseconds timeout);

private:
    [[nodiscard]] MatchCreated finalize_match(PlayerSession* white, PlayerSession* black);

    MatchmakingQueue queue_;
    IMatchCreatedHandler& handler_;
};

}  // namespace kfc
