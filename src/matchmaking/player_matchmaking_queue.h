#pragma once

#include "app/matchmaking_config.h"
#include "matchmaking/queued_player.h"

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace kfc {

class IRuntimeStore;

namespace matchmaking {

class PlayerMatchmakingQueue {
public:
    PlayerMatchmakingQueue(IRuntimeStore& runtime_store, std::string region,
                           app::MatchmakingConfig config);

    void remove(PlayerId player_id);
    [[nodiscard]] std::optional<std::pair<QueuedPlayer, QueuedPlayer>> try_pair(
        const QueuedPlayer& incoming);
    void enqueue(const QueuedPlayer& player);
    [[nodiscard]] std::vector<PlayerId> check_timeouts();
    [[nodiscard]] std::size_t waiting_count() const;

    [[nodiscard]] static std::int64_t current_epoch_seconds();

private:
    void persist_queue();
    [[nodiscard]] bool are_compatible(const QueuedPlayer& a, const QueuedPlayer& b) const;

    IRuntimeStore& runtime_store_;
    std::string region_;
    int max_rating_difference_;
    std::chrono::milliseconds queue_timeout_;
    std::vector<QueuedPlayer> waiting_;
};

}  // namespace matchmaking
}  // namespace kfc
