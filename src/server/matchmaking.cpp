#include "server/matchmaking.h"

#include <algorithm>

namespace kfc {

void Matchmaking::enqueue(PlayerSession& session) {
    waiting_.push_back(&session);
}

void Matchmaking::remove(PlayerSession& session) {
    waiting_.erase(
        std::remove(waiting_.begin(), waiting_.end(), &session),
        waiting_.end());
}

std::optional<std::vector<PlayerSession*>> Matchmaking::try_create_match() {
    if (waiting_.size() < 2) {
        return std::nullopt;
    }

    std::vector<PlayerSession*> matched{waiting_[0], waiting_[1]};
    waiting_.erase(waiting_.begin(), waiting_.begin() + 2);
    return matched;
}

}  // namespace kfc
