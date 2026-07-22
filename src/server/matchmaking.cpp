#include "server/matchmaking.h"

#include <algorithm>
#include <cmath>

namespace kfc {

void Matchmaking::remove(PlayerSession& session) {
    waiting_.erase(
        std::remove_if(
            waiting_.begin(),
            waiting_.end(),
            [&](const WaitingEntry& entry) { return entry.session == &session; }),
        waiting_.end());
}

std::optional<std::vector<PlayerSession*>> Matchmaking::enqueue(
    PlayerSession& session,
    const std::chrono::steady_clock::time_point now) {
    if (!is_eligible(session)) {
        return std::nullopt;
    }

    for (auto it = waiting_.begin(); it != waiting_.end(); ++it) {
        if (!is_eligible(*it->session)) {
            continue;
        }
        if (are_compatible(session, *it->session)) {
            PlayerSession* white = it->session;
            PlayerSession* black = &session;
            waiting_.erase(it);
            return std::vector<PlayerSession*>{white, black};
        }
    }

    waiting_.push_back(WaitingEntry{&session, now});
    return std::nullopt;
}

std::vector<PlayerSession*> Matchmaking::check_timeouts(
    const std::chrono::steady_clock::time_point now) {
    std::vector<PlayerSession*> timed_out;

    for (auto it = waiting_.begin(); it != waiting_.end();) {
        if (!is_eligible(*it->session)) {
            ++it;
            continue;
        }

        if (now - it->enqueue_time >= kQueueTimeout) {
            timed_out.push_back(it->session);
            it = waiting_.erase(it);
        } else {
            ++it;
        }
    }

    return timed_out;
}

std::size_t Matchmaking::waiting_count() const noexcept {
    return waiting_.size();
}

bool Matchmaking::are_compatible(const PlayerSession& a, const PlayerSession& b) {
    return std::abs(a.player().rating() - b.player().rating()) <= kMaxRatingDifference;
}

bool Matchmaking::is_eligible(const PlayerSession& session) {
    const ClientConnection* connection = session.connection();
    return session.has_player() && connection != nullptr && connection->is_open();
}

}  // namespace kfc
