#include "server/gateway/gateway_matchmaking_disconnect_handler.h"

#include "server/network/player_id.h"
#include "server/player_session.h"

namespace kfc {

GatewayMatchmakingDisconnectHandler::GatewayMatchmakingDisconnectHandler(
    matchmaking::IMatchmakingJoinClient& client, std::string region)
    : client_(client), region_(std::move(region)) {}

void GatewayMatchmakingDisconnectHandler::on_session_disconnected(PlayerSession& session) {
    if (session.state() == PlayerSessionState::Searching) {
        client_.leave(static_cast<PlayerId>(session.id()), region_);
    }
}

}  // namespace kfc
