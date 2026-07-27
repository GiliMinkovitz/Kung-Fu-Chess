#include "server/matchmaking/matchmaking_service.h"

#include "model/piece.h"
#include "server/player_session.h"

namespace kfc {

MatchmakingService::MatchmakingService(IMatchCreatedHandler& handler) : handler_(handler) {}

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

#ifdef KFC_TEST_BUILD
void MatchmakingService::set_queue_timeout(const std::chrono::milliseconds timeout) {
    queue_.set_queue_timeout(timeout);
}
#endif

MatchCreated MatchmakingService::finalize_match(PlayerSession* white, PlayerSession* black) {
    white->set_playing();
    black->set_playing();
    white->set_side(PieceColor::White);
    black->set_side(PieceColor::Black);

    const RoomId room_id = handler_.create_match(white, black);

    white->assign_room(room_id);
    black->assign_room(room_id);

    return MatchCreated{white, black, room_id};
}

}  // namespace kfc
