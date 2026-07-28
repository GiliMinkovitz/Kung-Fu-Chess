#include "server/matchmaking/matchmaking_service.h"

#include "server/player_session.h"

namespace kfc {

MatchmakingService::MatchmakingService(IMatchCreatedHandler& handler,
                                       const app::MatchmakingConfig& config)
    : queue_(config.max_rating_difference,
             std::chrono::duration_cast<std::chrono::milliseconds>(config.queue_timeout)),
      handler_(handler) {}

std::optional<MatchCreated> MatchmakingService::enqueue(
    PlayerSession& session,
    const std::chrono::steady_clock::time_point now) {
    if (const auto paired = queue_.enqueue(session, now)) {
        return finalize_match((*paired)[0], (*paired)[1]);
    }
    return std::nullopt;
}

void MatchmakingService::remove(PlayerSession& session) {
    queue_.remove(session);
}

std::vector<PlayerSession*> MatchmakingService::check_timeouts(
    const std::chrono::steady_clock::time_point now) {
    return queue_.check_timeouts(now);
}

std::size_t MatchmakingService::waiting_count() const noexcept {
    return queue_.waiting_count();
}

void MatchmakingService::set_queue_timeout(const std::chrono::milliseconds timeout) {
    queue_.set_queue_timeout(timeout);
}

MatchCreated MatchmakingService::finalize_match(PlayerSession* white, PlayerSession* black) {
    const RoomId room_id = handler_.create_match(white, black);
    return MatchCreated{white, black, room_id};
}

}  // namespace kfc
