#include "matchmaking/player_matchmaking_queue.h"

#include "app/i_runtime_store.h"

#include <algorithm>
#include <cmath>

namespace kfc::matchmaking {

PlayerMatchmakingQueue::PlayerMatchmakingQueue(IRuntimeStore& runtime_store, std::string region,
                                             const app::MatchmakingConfig config)
    : runtime_store_(runtime_store),
      region_(std::move(region)),
      max_rating_difference_(config.max_rating_difference),
      queue_timeout_(std::chrono::duration_cast<std::chrono::milliseconds>(config.queue_timeout)),
      waiting_(runtime_store_.list_matchmaking_queue(region_)) {}

void PlayerMatchmakingQueue::persist_queue() {
    runtime_store_.set_matchmaking_queue(region_, waiting_);
}

void PlayerMatchmakingQueue::remove(const PlayerId player_id) {
    waiting_.erase(std::remove_if(waiting_.begin(), waiting_.end(),
                                    [player_id](const QueuedPlayer& entry) {
                                        return entry.player_id == player_id;
                                    }),
                     waiting_.end());
    persist_queue();
}

std::optional<std::pair<QueuedPlayer, QueuedPlayer>> PlayerMatchmakingQueue::try_pair(
    const QueuedPlayer& incoming) {
    for (auto it = waiting_.begin(); it != waiting_.end(); ++it) {
        if (are_compatible(incoming, *it)) {
            QueuedPlayer white = *it;
            QueuedPlayer black = incoming;
            waiting_.erase(it);
            persist_queue();
            return std::make_pair(white, black);
        }
    }

    return std::nullopt;
}

void PlayerMatchmakingQueue::enqueue(const QueuedPlayer& player) {
    waiting_.push_back(player);
    persist_queue();
}

std::vector<PlayerId> PlayerMatchmakingQueue::check_timeouts() {
    const std::int64_t now = current_epoch_seconds();
    std::vector<PlayerId> timed_out;

    for (auto it = waiting_.begin(); it != waiting_.end();) {
        if (now - it->enqueue_epoch_seconds >= queue_timeout_.count() / 1000) {
            timed_out.push_back(it->player_id);
            it = waiting_.erase(it);
        } else {
            ++it;
        }
    }

    if (!timed_out.empty()) {
        persist_queue();
    }
    return timed_out;
}

std::size_t PlayerMatchmakingQueue::waiting_count() const {
    return waiting_.size();
}

bool PlayerMatchmakingQueue::are_compatible(const QueuedPlayer& a,
                                          const QueuedPlayer& b) const {
    return std::abs(a.elo - b.elo) <= max_rating_difference_;
}

std::int64_t PlayerMatchmakingQueue::current_epoch_seconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

}  // namespace kfc::matchmaking
