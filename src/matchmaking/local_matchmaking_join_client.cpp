#include "matchmaking/local_matchmaking_join_client.h"

#include "server/matchmaking/match_created_handler.h"
#include "server/player_session.h"

#include <chrono>

namespace kfc::matchmaking {

LocalMatchmakingJoinClient::LocalMatchmakingJoinClient(
    MatchmakingService& matchmaking_service, MatchLifecycleHandler& match_lifecycle_handler,
    ClientSessionManager& session_manager)
    : matchmaking_service_(matchmaking_service),
      match_lifecycle_handler_(match_lifecycle_handler),
      session_manager_(session_manager) {}

std::optional<MatchResponse> LocalMatchmakingJoinClient::join(const MatchRequest& request) {
    PlayerSession* session = session_manager_.find_session(request.player_id);
    if (session == nullptr) {
        return MatchResponse{MatchJoinStatus::Error, "session_not_found"};
    }

    const auto now = std::chrono::steady_clock::now();
    if (const std::optional<MatchCreated> match = matchmaking_service_.enqueue(*session, now)) {
        match_lifecycle_handler_.notify_match_created(*match);
        return MatchResponse{MatchJoinStatus::Queued, "matched"};
    }

    return MatchResponse{MatchJoinStatus::Queued, "queued"};
}

void LocalMatchmakingJoinClient::leave(const PlayerId player_id, const std::string_view) {
    if (PlayerSession* session = session_manager_.find_session(player_id)) {
        matchmaking_service_.remove(*session);
    }
}

}  // namespace kfc::matchmaking
