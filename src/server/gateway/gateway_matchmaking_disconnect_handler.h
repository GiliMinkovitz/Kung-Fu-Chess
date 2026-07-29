#pragma once

#include "matchmaking/i_matchmaking_join_client.h"
#include "server/session/i_session_disconnect_handler.h"

#include <string>

namespace kfc {

class GatewayMatchmakingDisconnectHandler final : public ISessionDisconnectHandler {
public:
    GatewayMatchmakingDisconnectHandler(matchmaking::IMatchmakingJoinClient& client,
                                        std::string region);

    void on_session_disconnected(PlayerSession& session) override;

private:
    matchmaking::IMatchmakingJoinClient& client_;
    std::string region_;
};

}  // namespace kfc
