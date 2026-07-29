#pragma once

#include "matchmaking/i_matchmaking_join_client.h"
#include "server/match/match_lifecycle_handler.h"
#include "server/matchmaking/matchmaking_service.h"
#include "server/session/client_session_manager.h"

namespace kfc::matchmaking {

class LocalMatchmakingJoinClient final : public IMatchmakingJoinClient {
public:
    LocalMatchmakingJoinClient(MatchmakingService& matchmaking_service,
                               MatchLifecycleHandler& match_lifecycle_handler,
                               ClientSessionManager& session_manager);

    [[nodiscard]] std::optional<MatchResponse> join(const MatchRequest& request) override;
    void leave(PlayerId player_id, std::string_view region) override;

private:
    MatchmakingService& matchmaking_service_;
    MatchLifecycleHandler& match_lifecycle_handler_;
    ClientSessionManager& session_manager_;
};

}  // namespace kfc::matchmaking
