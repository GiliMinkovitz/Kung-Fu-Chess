#pragma once

#include "matchmaking/protocol/match_notification.h"
#include "server/network/player_id.h"

namespace kfc {

class ClientSessionManager;
class LocalGameGateway;

class GatewayNotificationHandler {
public:
    GatewayNotificationHandler(ClientSessionManager& session_manager,
                               LocalGameGateway& game_gateway);

    void notify_match_found(const matchmaking::MatchNotification& notification);
    void notify_search_timeout(PlayerId player_id);

private:
    ClientSessionManager& session_manager_;
    LocalGameGateway& game_gateway_;
};

}  // namespace kfc
