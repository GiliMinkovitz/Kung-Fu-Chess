#include "server/gateway/monolith_matchmaking_disconnect_handler.h"

#include "server/matchmaking/matchmaking_service.h"
#include "server/player_session.h"

namespace kfc {

MonolithMatchmakingDisconnectHandler::MonolithMatchmakingDisconnectHandler(
    MatchmakingService& matchmaking_service)
    : matchmaking_service_(matchmaking_service) {}

void MonolithMatchmakingDisconnectHandler::on_session_disconnected(PlayerSession& session) {
    if (session.state() == PlayerSessionState::Searching) {
        matchmaking_service_.remove(session);
    }
}

}  // namespace kfc
